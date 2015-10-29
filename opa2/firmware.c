/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 *
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
#include <linux/sched.h>
#include "opa_hfi.h"
#include "debugfs.h"
#include "link.h"
#include "rdma/fxr/mnh_8051_defs.h"
#include "rdma/fxr/fxr_fc_defs.h"
#include "firmware.h"
#include "rdma/fxr/mnh_misc_defs.h"

/*
 * Make it easy to toggle firmware file name and if it gets loaded by
 * editing the following. This may be something we do while in development
 * but not necessarily something a user would ever need to use.
 */
#define DEFAULT_FW_8051_NAME_FPGA "hfi_dc8051.bin"
#define ALT_FW_8051_NAME_ASIC "hfi1_dc8051_d.fw"

/* expected field values */
#define CSS_MODULE_TYPE	   0x00000006
#define CSS_HEADER_LEN	   0x000000a1
#define CSS_HEADER_VERSION 0x00010000
#define CSS_MODULE_VENDOR  0x00008086

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
	write_8051_csr(ppd, CRK_CRK8051_CFG_RAM_ACCESS_CTRL, reg);

	/* wait until ACCESS_COMPLETED is set */
	count = 0;
	while ((read_8051_csr(ppd, CRK_CRK8051_CFG_RAM_ACCESS_STATUS)
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
	*result = read_8051_csr(ppd, CRK_CRK8051_CFG_RAM_ACCESS_RD_DATA);

	return 0;
}

/*
 * Read 8051 data starting at addr, for len bytes.  Will read in 8-byte chunks.
 * Return 0 on success, -errno on error.
 */
static int read_8051_data(struct hfi_pportdata *ppd, u32 addr, u32 len, u64 *result)
{
	unsigned long flags;
	u32 done;
	int ret = 0;

	spin_lock_irqsave(&ppd->crk8051_lock, flags);

	/* data read set-up, no auto-increment */
	write_8051_csr(ppd, CRK_CRK8051_CFG_RAM_ACCESS_SETUP, 0);

	for (done = 0; done < len; addr += 8, done += 8, result++) {
		ret = __read_8051_data(ppd, addr, result);
		if (ret)
			break;
	}

	/* turn off read enable */
	write_8051_csr(ppd, CRK_CRK8051_CFG_RAM_ACCESS_CTRL, 0);

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
	write_8051_csr(ppd, CRK_CRK8051_CFG_RAM_ACCESS_SETUP, reg);

	reg = ((start & CRK_CRK8051_CFG_RAM_ACCESS_CTRL_ADDRESS_MASK)
			<< CRK_CRK8051_CFG_RAM_ACCESS_CTRL_ADDRESS_SHIFT)
		| CRK_CRK8051_CFG_RAM_ACCESS_CTRL_WRITE_ENA_SMASK;
	write_8051_csr(ppd, CRK_CRK8051_CFG_RAM_ACCESS_CTRL, reg);

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
		write_8051_csr(ppd, CRK_CRK8051_CFG_RAM_ACCESS_WR_DATA, reg);

		/* wait until ACCESS_COMPLETED is set */
		count = 0;
		while ((read_8051_csr(ppd, CRK_CRK8051_CFG_RAM_ACCESS_STATUS)
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
	write_8051_csr(ppd, CRK_CRK8051_CFG_RAM_ACCESS_CTRL, 0);
	write_8051_csr(ppd, CRK_CRK8051_CFG_RAM_ACCESS_SETUP, 0);

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

/*
 * Request the firmware from the system.  Extract the pieces and fill in
 * fdet.  If successful, the caller will need to call dispose_one_firmware().
 * Returns 0 on success, -ERRNO on error.
 */
static int obtain_one_firmware(struct hfi_devdata *dd, const char *name,
			       struct firmware_details *fdet)
{
	struct css_header *css;
	int ret;

	memset(fdet, 0, sizeof(*fdet));

	ret = request_firmware(&fdet->fw, name, &dd->pcidev->dev);
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
	 * If the file does not have a valid CSS header, fail.
	 * Otherwise, check the CSS size field for an expected size.
	 * The augmented file has r2 and mu inserted after the header
	 * was generated, so there will be a known difference between
	 * the CSS header size and the actual file size.  Use this
	 * difference to identify an augmented file.
	 *
	 * Note: css->size is in DWORDs, multiply by 4 to get bytes.
	 */
	ret = verify_css_header(dd, css);
	if (ret) {
		dd_dev_info(dd, "Invalid CSS header for \"%s\"\n", name);
	} else {
		if ((css->size * 4) == fdet->fw->size) {
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
				 * generate here.
				 * For now, fail.
				 */
				dd_dev_err(dd, "driver is unable to validate firmware without r2 and mu (not in firmware file)\n");
				ret = -EINVAL;
			}
		} else {
			if ((css->size * 4) + AUGMENT_SIZE == fdet->fw->size) {
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
		}
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
static void __obtain_firmware(struct hfi_devdata *dd)
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
		if (dd->fw_8051_load)
			dispose_one_firmware(&dd->fw_8051);
		dd->fw_8051_name = ALT_FW_8051_NAME_ASIC;
	}

	if (dd->fw_8051_load) {
		err = obtain_one_firmware(dd, dd->fw_8051_name, &dd->fw_8051);
		if (err)
			goto done;
	}

done:
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
static int obtain_firmware(struct hfi_devdata *dd)
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

	if (dd->fw_state == FW_FINAL) {
		goto done;	/* already acquired */
	} else if (dd->fw_state == FW_ERR) {
		goto done;	/* already tried and failed */
	}
	/* fw_state is FW_EMPTY */

	/* set fw_state to FW_TRY, FW_FINAL, or FW_ERR, and fw_err */
	__obtain_firmware(dd);

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
	dispose_one_firmware(&dd->fw_8051);

	/* retain the error state, otherwise revert to empty */
	if (dd->fw_state != FW_ERR)
		dd->fw_state = FW_EMPTY;
}

/*
 * Called with the result of a firmware download.
 *
 * Return 1 to retry loading the firmware, 0 to stop.
 */
static int retry_firmware(struct hfi_devdata *dd, int load_result)
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
		__obtain_firmware(dd);
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
	u32 status;
	int ret = 0;

	/* write the signature */
	write_rsa_data(dd, FXR_MNH_MISC_CSRS + MNH_MISC_CFG_RSA_SIGNATURE,
		signature, KEY_SIZE);

	/* initialize RSA */
	write_csr(dd, FXR_MNH_MISC_CSRS + MNH_MISC_CFG_RSA_CMD, RSA_CMD_INIT);

	/*
	 * Make sure the engine is idle and insert a delay between the two
	 * writes to MNH_MISC_CFG_RSA_CMD.
	 */
	status = (read_csr(dd, FXR_MNH_MISC_CSRS + MNH_MISC_STS_FW)
			   & MNH_MISC_STS_FW_RSA_STATUS_SMASK)
			     >> MNH_MISC_STS_FW_RSA_STATUS_SHIFT;
	if (status != RSA_STATUS_IDLE) {
		ppd_dev_err(ppd, "%s security engine not idle - giving up\n",
			   who);
		return -EBUSY;
	}

	/* start RSA */
	write_csr(dd, FXR_MNH_MISC_CSRS + MNH_MISC_CFG_RSA_CMD, RSA_CMD_START);

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
		status = (read_csr(dd, FXR_MNH_MISC_CSRS + MNH_MISC_CFG_FW_CTRL)
			   & MNH_MISC_STS_FW_RSA_STATUS_SMASK)
			     >> MNH_MISC_STS_FW_RSA_STATUS_SHIFT;

#if 1 /* will fix by STL-3430 */
		if (status == RSA_STATUS_IDLE || status == RSA_STATUS_DONE) {
#else
		if (status == RSA_STATUS_IDLE) {
			/* should not happen */
			ppd_dev_err(ppd, "%s firmware security bad idle state\n",
				   who);
			ret = -EINVAL;
			break;
		} else if (status == RSA_STATUS_DONE) {
#endif
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
			ppd_dev_err(ppd, "%s firmware security time out\n", who);
			ret = -ETIMEDOUT;
			break;
		}

		msleep(20);
	}

	return ret;
}

static void load_security_variables(struct hfi_devdata *dd,
				    struct firmware_details *fdet)
{
	/* Security variables a.  Write the modulus */
	write_rsa_data(dd, FXR_MNH_MISC_CSRS + MNH_MISC_CFG_RSA_MODULUS,
		fdet->modulus, KEY_SIZE);
	/* Security variables b.  Write the r2 */
	write_rsa_data(dd, FXR_MNH_MISC_CSRS + MNH_MISC_CFG_RSA_R2,
		fdet->r2, KEY_SIZE);
	/* Security variables c.  Write the mu */
	write_rsa_data(dd, FXR_MNH_MISC_CSRS + MNH_MISC_CFG_RSA_MU,
		fdet->mu, MU_SIZE);
	/* Security variables d.  Write the header */
	write_streamed_rsa_data(dd, FXR_MNH_MISC_CSRS + MNH_MISC_CFG_SHA_PRELOAD,
				(u8 *)fdet->css_header,
				sizeof(struct css_header));
}

/* return the 8051 firmware state */
static inline u32 get_firmware_state(const struct hfi_pportdata *ppd)
{
	u64 reg = read_8051_csr(ppd, CRK_CRK8051_STS_CUR_STATE);

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
static int read_8051_config(struct hfi_pportdata *ppd, u8 field_id, u8 lane_id,
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
	ret = read_8051_data(ppd, addr, 8, &big_data);

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

static void read_misc_status(struct hfi_pportdata *ppd, u8 *ver_a, u8 *ver_b)
{
	u32 frame;

	read_8051_config(ppd, MISC_STATUS, GENERAL_CONFIG, &frame);
	*ver_a = (frame >> STS_FM_VERSION_A_SHIFT) & STS_FM_VERSION_A_MASK;
	*ver_b = (frame >> STS_FM_VERSION_B_SHIFT) & STS_FM_VERSION_B_MASK;
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
	write_8051_csr(ppd, CRK_CRK8051_CFG_RST, reg);

	/*
	 * MNH reset step 2 (optional): Load 8051 data memory with link
	 * configuration
	 */

	/*
	 * MNH reset step 3: Load 8051 firmware
	 */
	/* release all but the core reset */
	reg = CRK_CRK8051_CFG_RST_M8051W_SMASK;
	write_8051_csr(ppd, CRK_CRK8051_CFG_RST, reg);

	/* Firmware load step 1 */
	load_security_variables(dd, fdet);

	/*
	 * Firmware load step 2.  Clear MNH_MISC_CFG_FW_CTRL.FW_8051_LOADED
	 */
	write_csr(dd, FXR_MNH_MISC_CSRS + MNH_MISC_CFG_FW_CTRL, 0);

	/* Firmware load steps 3-5 */
	ret = write_8051(ppd, 1/*code*/, 0, fdet->firmware_ptr,
			 fdet->firmware_len);
	if (ret)
		return ret;

	/*
	 * MNH reset step 4. Host starts the 8051 firmware
	 */
	/*
	 * Firmware load step 6.  Set MNH_MISC_CFG_FW_CTRL.FW_8051_LOADED
	 * There is simpler way where pnum is 1 or 2 as its prerequisite.
	 * But I prefer the way to remove prerequisite.
	 */
	switch (ppd->pnum) {
	case 1:
		reg = 0x01; break;
	case 2:
		reg = 0x02; break;
	default:
		ppd_dev_err(ppd, "invalid pnum: %d", ppd->pnum);
		return -EINVAL;
	}
	write_csr(dd, FXR_MNH_MISC_CSRS + MNH_MISC_CFG_FW_CTRL,
		reg << MNH_MISC_CFG_FW_CTRL_FW_8051_LOADED_SHIFT);

	/* Firmware load steps 7-10 */
	ret = run_rsa(ppd, "8051", fdet->signature);
	if (ret)
		return ret;

	/* clear all reset bits, releasing the 8051 */
	write_8051_csr(ppd, CRK_CRK8051_CFG_RST, 0ull);

	/*
	 * MNH reset step 5. Wait for firmware to be ready to accept host
	 * requests.
	 */
	ret = hfi_wait_firmware_ready(ppd, TIMEOUT_8051_START);
	if (ret) { /* timed out */
		ppd_dev_err(ppd, "8051 start timeout, current state 0x%x\n",
			   get_firmware_state(ppd));
		return -ETIMEDOUT;
	}

	return 0;
}

static int _load_firmware(struct hfi_pportdata *ppd)
{
	int ret;

	if (ppd->dd->fw_8051_load) {
		do {
			ret = load_8051_firmware(ppd, &ppd->dd->fw_8051);
		} while (retry_firmware(ppd->dd, ret));
		if (ret)
			return ret;
	}

	return 0;
}

static int firmware_init(struct hfi_devdata *dd)
{
	if (!dd->fw_8051_name)
		dd->fw_8051_name = DEFAULT_FW_8051_NAME_FPGA;

	return obtain_firmware(dd);
}

/*
 */
int hfi2_load_firmware(struct hfi_devdata *dd)
{
	int ret;
	u8 port;
	struct hfi_pportdata *ppd;
	u8 ver_a, ver_b;

	mutex_init(&dd->fw_mutex);
	dd->fw_state = FW_EMPTY;
	dd->fw_8051_load = true;
	ret = firmware_init(dd);
	if (ret) {
		dd_dev_err(dd, "can't init firmware");
		return ret;
	}
	for (port = 1; port <= dd->num_pports; port++) {
		ppd = to_hfi_ppd(dd, port);
		ret = _load_firmware(ppd);
		if (ret) {
			ppd_dev_err(ppd, "can't load firmware");
			return ret;
		}
	}

	read_misc_status(to_hfi_ppd(dd, 1), &ver_a, &ver_b);
	dd_dev_info(dd, "8051 firmware version %d.%d\n",
		    (int)ver_b, (int)ver_a);
	dd->crk8051_ver = crk8051_ver(ver_b, ver_a);

	return 0;
}

void hfi2_dispose_firmware(struct hfi_devdata *dd)
{
	dispose_firmware(dd);
}
