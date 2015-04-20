/*
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2015 Intel Corporation.
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
 * Copyright(c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Intel Corporation nor the names of its
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
 */

#include <linux/firmware.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/delay.h>

#include "hfi.h"
#include "trace.h"

static uint fw_8051_load = 1;
module_param_named(fw_8051_load, fw_8051_load, uint, S_IRUGO);
MODULE_PARM_DESC(fw_8051_load, "Load the 8051 firmware");

static uint fw_fabric_serdes_load = 1;
module_param_named(fw_fabric_serdes_load, fw_fabric_serdes_load, uint, S_IRUGO);
MODULE_PARM_DESC(fw_fabric_serdes_load, "Load the fabric SerDes firmware");

static uint fw_pcie_serdes_load = 1;
module_param_named(fw_pcie_serdes_load, fw_pcie_serdes_load, uint, S_IRUGO);
MODULE_PARM_DESC(fw_pcie_serdes_load, "Load the PCIe SerDes firmware");

static uint fw_sbus_load = 1;
module_param_named(fw_sbus_load, fw_sbus_load, uint, S_IRUGO);
MODULE_PARM_DESC(fw_sbus_load, "Load the SBus firmware");

static uint fw_validate;
module_param_named(fw_validate, fw_validate, uint, S_IRUGO);
MODULE_PARM_DESC(fw_validate, "Perform firmware validation");

#define DEFAULT_FW_8051_NAME_FPGA "hfi_dc8051.bin"
#define DEFAULT_FW_8051_NAME_ASIC "hfi1_dc8051.fw"
#define DEFAULT_FW_FABRIC_NAME "hfi1_fabric.fw"
#define DEFAULT_FW_SBUS_NAME "hfi1_sbus.fw"
#define DEFAULT_FW_PCIE_NAME "hfi1_pcie.fw"

static char *fw_8051_name;
module_param_named(fw_8051_name, fw_8051_name, charp, S_IRUGO);
MODULE_PARM_DESC(fw_8051_name, "8051 firmware name");

static char *fw_fabric_serdes_name;
module_param_named(
	fw_fabric_serdes_name,
	fw_fabric_serdes_name,
	charp,
	S_IRUGO);
MODULE_PARM_DESC(fw_fabric_serdes_name, "Fabric SerDes firmware name");

static char *fw_sbus_name;
module_param_named(fw_sbus_name, fw_sbus_name, charp, S_IRUGO);
MODULE_PARM_DESC(fw_sbus_name, "SBus firmware name");

static char *fw_pcie_serdes_name;
module_param_named(fw_pcie_serdes_name, fw_pcie_serdes_name, charp, S_IRUGO);
MODULE_PARM_DESC(fw_pcie_serdes_name, "PCIe SerDes firmware name");

#define SBUS_MAX_POLL_COUNT 100
#define SBUS_COUNTER(reg, name) \
	(((reg) >> ASIC_STS_SBUS_COUNTERS_##name##_CNT_SHIFT) & \
	 ASIC_STS_SBUS_COUNTERS_##name##_CNT_MASK)

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
/* expected field values */
#define CSS_MODULE_TYPE	   0x00000006
#define CSS_HEADER_LEN	   0x000000a1
#define CSS_HEADER_VERSION 0x00010000
#define CSS_MODULE_VENDOR  0x00008086

#define KEY_SIZE      256
#define MU_SIZE		8
#define EXPONENT_SIZE	4

/* the file itself */
struct firmware_file {
	struct css_header css_header;
	u8 modulus[KEY_SIZE];
	u8 exponent[EXPONENT_SIZE];
	u8 signature[KEY_SIZE];
	u8 firmware[];
};

struct augmented_firmware_file {
	struct css_header css_header;
	u8 modulus[KEY_SIZE];
	u8 exponent[EXPONENT_SIZE];
	u8 signature[KEY_SIZE];
	u8 r2[KEY_SIZE];
	u8 mu[MU_SIZE];
	u8 firmware[];
};

/* augmented file size difference */
#define AUGMENT_SIZE (sizeof(struct augmented_firmware_file) - \
						sizeof(struct firmware_file))

struct firmware_details {
	/* linux core piece */
	const struct firmware *fw;

	struct css_header *css_header;
	u8 *firmware_ptr;		/* pointer to binary data */
	u32 firmware_len;		/* length in bytes */
	u8 *modulus;			/* pointer to the moduls */
	u8 *exponent;			/* pointer to the exponent */
	u8 *signature;			/* ponter to the signature */
	u8 *r2;				/* pointer to r2 */
	u8 *mu;				/* pointer to mu */
	struct augmented_firmware_file dummy_header;
};

/*
 * The mutex protects fw_state, fw_err, and all of the firmware_details
 * variables.
 */
static DEFINE_MUTEX(fw_mutex);
enum fw_state {
	FW_EMPTY,
	FW_ACQUIRED,
	FW_ERR
};
static enum fw_state fw_state = FW_EMPTY;
static int fw_err;
static struct firmware_details fw_8051;
static struct firmware_details fw_fabric;
static struct firmware_details fw_pcie;
static struct firmware_details fw_sbus;

/* flags for turn_off_spicos() */
#define SPICO_SBUS   0x1
#define SPICO_FABRIC 0x2

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
#define HM_TIMEOUT 20 /* ms */

/* 8051 memory access timout, in us */
#define DC8051_ACCESS_TIMEOUT 100 /* us */

/* the number of fabric SerDes on the SBus */
#define NUM_FABRIC_SERDES 4

/* SBus fabric SerDes addresses, one set per HFI */
static const u8 fabric_serdes_addrs[2][NUM_FABRIC_SERDES] = {
	{ 0x01, 0x02, 0x03, 0x04 },
	{ 0x28, 0x29, 0x2a, 0x2b }
};

/* SBus PCIe SerDes addresses, one set per HFI */
static const u8 pcie_serdes_addrs[2][NUM_PCIE_SERDES] = {
	{ 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x14, 0x16,
	  0x18, 0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x24, 0x26 },
	{ 0x2f, 0x31, 0x33, 0x35, 0x37, 0x39, 0x3b, 0x3d,
	  0x3f, 0x41, 0x43, 0x45, 0x47, 0x49, 0x4b, 0x4d }
};

/* SBus PCIe PCS addresses, one set per HFI */
const u8 pcie_pcs_addrs[2][NUM_PCIE_SERDES] = {
	{ 0x09, 0x0b, 0x0d, 0x0f, 0x11, 0x13, 0x15, 0x17,
	  0x19, 0x1b, 0x1d, 0x1f, 0x21, 0x23, 0x25, 0x27 },
	{ 0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e,
	  0x40, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e }
};

/* SBus fabric SerDes broadcast addresses, one per HFI */
static const u8 fabric_serdes_broadcast[2] = { 0xe4, 0xe5 };
static const u8 all_fabric_serdes_broadcast = 0xe1;

/* SBus PCIe SerDes broadcast addresses, one per HFI */
const u8 pcie_serdes_broadcast[2] = { 0xe2, 0xe3 };
static const u8 all_pcie_serdes_broadcast = 0xe0;

/* forwards */
static void dispose_one_firmware(struct firmware_details *fdet);

/*
 * Write data or code to the 8051 code or data RAM.
 */
static int write_8051(struct hfi_devdata *dd, int code, u32 start,
					const u8 *data, u32 len)
{
	u64 reg;
	u32 offset;
	int aligned, count;

	/* check alignment */
	aligned = ((unsigned long)data & 0x7) == 0;

	/* write set-up */
	reg = (code ? DC_DC8051_CFG_RAM_ACCESS_SETUP_RAM_SEL_SMASK : 0ull)
		| DC_DC8051_CFG_RAM_ACCESS_SETUP_AUTO_INCR_ADDR_SMASK;
	write_csr(dd, DC_DC8051_CFG_RAM_ACCESS_SETUP, reg);

	reg = ((start & DC_DC8051_CFG_RAM_ACCESS_CTRL_ADDRESS_MASK)
			<< DC_DC8051_CFG_RAM_ACCESS_CTRL_ADDRESS_SHIFT)
		| DC_DC8051_CFG_RAM_ACCESS_CTRL_WRITE_ENA_SMASK;
	write_csr(dd, DC_DC8051_CFG_RAM_ACCESS_CTRL, reg);

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
		write_csr(dd, DC_DC8051_CFG_RAM_ACCESS_WR_DATA, reg);

		/* wait until ACCESS_COMPLETED is set */
		count = 0;
		while ((read_csr(dd, DC_DC8051_CFG_RAM_ACCESS_STATUS)
		    & DC_DC8051_CFG_RAM_ACCESS_STATUS_ACCESS_COMPLETED_SMASK)
		    == 0) {
			count++;
			if (count > DC8051_ACCESS_TIMEOUT) {
				dd_dev_err(dd, "timeout writing 8051 data\n");
				return -ENXIO;
			}
			udelay(1);
		}
	}

	/* turn off write access, auto increment (also sets to data access) */
	write_csr(dd, DC_DC8051_CFG_RAM_ACCESS_CTRL, 0);
	write_csr(dd, DC_DC8051_CFG_RAM_ACCESS_SETUP, 0);

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
	if (invalid_header(dd, "module_type", css->module_type, CSS_MODULE_TYPE)
			|| invalid_header(dd, "header_len", css->header_len,
					(sizeof(struct firmware_file)/4))
			|| invalid_header(dd, "header_version",
					css->header_version, CSS_HEADER_VERSION)
			|| invalid_header(dd, "module_vendor",
					css->module_vendor, CSS_MODULE_VENDOR)
			|| invalid_header(dd, "key_size",
					css->key_size, KEY_SIZE/4)
			|| invalid_header(dd, "modulus_size",
					css->modulus_size, KEY_SIZE/4)
			|| invalid_header(dd, "exponent_size",
					css->exponent_size, EXPONENT_SIZE/4)) {
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
 * fdet.  If succsessful, the caller will need to call dispose_one_firmware().
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
		dd_dev_err(dd, "cannot load firmware \"%s\", err %d\n",
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

	hfi_cdbg(FIRMWARE, "Firmware %s details:", name);
	hfi_cdbg(FIRMWARE, "file size: 0x%lx bytes", fdet->fw->size);
	hfi_cdbg(FIRMWARE, "CSS structure:");
	hfi_cdbg(FIRMWARE, "  module_type    0x%x", css->module_type);
	hfi_cdbg(FIRMWARE, "  header_len     0x%03x (0x%03x bytes)",
		css->header_len, 4*css->header_len);
	hfi_cdbg(FIRMWARE, "  header_version 0x%x", css->header_version);
	hfi_cdbg(FIRMWARE, "  module_id      0x%x", css->module_id);
	hfi_cdbg(FIRMWARE, "  module_vendor  0x%x", css->module_vendor);
	hfi_cdbg(FIRMWARE, "  date           0x%x", css->date);
	hfi_cdbg(FIRMWARE, "  size           0x%03x (0x%03x bytes)",
		css->size, 4*css->size);
	hfi_cdbg(FIRMWARE, "  key_size       0x%03x (0x%03x bytes)",
		css->key_size, 4*css->key_size);
	hfi_cdbg(FIRMWARE, "  modulus_size   0x%03x (0x%03x bytes)",
		css->modulus_size, 4*css->modulus_size);
	hfi_cdbg(FIRMWARE, "  exponent_size  0x%03x (0x%03x bytes)",
		css->exponent_size, 4*css->exponent_size);
	hfi_cdbg(FIRMWARE, "firmware size: 0x%lx bytes",
		fdet->fw->size - sizeof(struct firmware_file));

	/*
	 * If the file does not have a valid CSS header, assume it is
	 * a raw binary.  Otherwise, check the CSS size field for an
	 * expected size.  The augmented file has r2 and mu inserted
	 * after the header was genereated, so there will be a known
	 * difference between the CSS header size and the actual file
	 * size.  Use this difference to identify an augmented file.
	 *
	 * Note: css->size is in DWORDs, multiply by 4 to get bytes.
	 */
	ret = verify_css_header(dd, css);
	if (ret) {
		/* assume this is a raw binary, with no CSS header */
		dd_dev_info(dd,
			"Invalid CSS header for \"%s\" - assuming raw binary, turning off validation",
			name);
		ret = 0; /* now OK */
		fw_validate = 0;
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
	} else if ((css->size*4) == fdet->fw->size) {
		/* non-agumented firmware file */
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
			fdet->r2 = fdet->dummy_header.r2; /* use dummy space */
			fdet->mu = fdet->dummy_header.mu; /* use dummy space */
			fdet->firmware_ptr = ff->firmware;
			fdet->firmware_len = fdet->fw->size -
						sizeof(struct firmware_file);
			/*
			 * Header does not include r2 and mu - generate here.
			 * For now, fail if validating.
			 */
			if (fw_validate) {
				dd_dev_err(dd, "driver is unable to validate firmware without r2 and mu (not in firmware file)\n");
				ret = -EINVAL;
			}
		}
	} else if ((css->size*4) + AUGMENT_SIZE == fdet->fw->size) {
		/* agumented firmware file */
		struct augmented_firmware_file *aff =
			(struct augmented_firmware_file *)fdet->fw->data;

		/* make sure there are bytes in the payload */
		ret = payload_check(dd, name, fdet->fw->size,
					sizeof(struct augmented_firmware_file));
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
			fdet->fw->size/4, (fdet->fw->size - AUGMENT_SIZE)/4,
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
	if (fdet->fw) {
		release_firmware(fdet->fw);
		fdet->fw = NULL;
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
	int err = 0;

	mutex_lock(&fw_mutex);
	if (fw_state == FW_ACQUIRED) {
		goto done;	/* already acquired */
	} else if (fw_state == FW_ERR) {
		err = fw_err;
		goto done;	/* already tried and failed */
	}

	if (fw_8051_load) {
		err = obtain_one_firmware(dd, fw_8051_name, &fw_8051);
		if (err)
			goto done;
	}

	if (fw_fabric_serdes_load) {
		err = obtain_one_firmware(dd, fw_fabric_serdes_name,
			&fw_fabric);
		if (err)
			goto done;
	}

	if (fw_sbus_load) {
		err = obtain_one_firmware(dd, fw_sbus_name, &fw_sbus);
		if (err)
			goto done;
	}

	if (fw_pcie_serdes_load) {
		err = obtain_one_firmware(dd, fw_pcie_serdes_name, &fw_pcie);
		if (err)
			goto done;
	}

	/* success */
	fw_state = FW_ACQUIRED;

done:
	if (err) {
		fw_err = err;
		fw_state = FW_ERR;
	}
	mutex_unlock(&fw_mutex);

	return err;
}

/*
 * Called when the driver unloads.  The timing is asymmetric with its
 * counterpart, obtain_firmware().  If called at device remove time,
 * then it is concievable that another device could probe while the
 * firmware is being disposed.  The mutexes can be moved to do that
 * safely, but then the firmware would be requested from the OS multiple
 * times.
 *
 * No mutex is needed as the driver is unloading and there cannot be any
 * other callers.
 */
void dispose_firmware(void)
{
	dispose_one_firmware(&fw_8051);
	dispose_one_firmware(&fw_fabric);
	dispose_one_firmware(&fw_pcie);
	dispose_one_firmware(&fw_sbus);
	/* retain the error state, otherwise revert to empty */
	if (fw_state != FW_ERR)
		fw_state = FW_EMPTY;
}

/*
 * Write a block of data to a given array CSR.  All calls will be in
 * multiples of 8 bytes.
 */
static void write_rsa_data(struct hfi_devdata *dd, int what,
				const u8 *data, int nbytes)
{
	int qw_size = nbytes/8;
	int i;

	if (((unsigned long)data & 0x7) == 0) {
		/* aligned */
		u64 *ptr = (u64 *)data;

		for (i = 0; i < qw_size; i++, ptr++)
			write_csr(dd, what + (8*i), *ptr);
	} else {
		/* not aligned */
		for (i = 0; i < qw_size; i++, data += 8) {
			u64 value;

			memcpy(&value, data, 8);
			write_csr(dd, what + (8*i), value);
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
	int qw_size = nbytes/8;

	for (; qw_size > 0; qw_size--, ptr++)
		write_csr(dd, what, *ptr);
}

/*
 * Download the signature and start the RSA mechanism.  Wait for
 * RSA_ENGINE_TIMEOUT before giving up.
 */
static int run_rsa(struct hfi_devdata *dd, const char *who, const u8 *signature)
{
	unsigned long timeout;
	u64 reg;
	u32 status;
	int ret = 0;

	if (!fw_validate)
		return 0;	/* done with no error if not validating */

	/* write the signature */
	write_rsa_data(dd, MISC_CFG_RSA_SIGNATURE, signature, KEY_SIZE);

	/* init RSA */
	write_csr(dd, MISC_CFG_RSA_CMD, RSA_CMD_INIT);

	/*
	 * Make sure the engine is idle and insert a delay between the two
	 * writes to MISC_CFG_RSA_CMD.
	 */
	status = (read_csr(dd, MISC_CFG_FW_CTRL)
			   & MISC_CFG_FW_CTRL_RSA_STATUS_SMASK)
			     >> MISC_CFG_FW_CTRL_RSA_STATUS_SHIFT;
	if (status != RSA_STATUS_IDLE) {
		dd_dev_err(dd, "%s security engine not idle - giving up\n",
			who);
		return -EBUSY;
	}

	/* start RSA */
	write_csr(dd, MISC_CFG_RSA_CMD, RSA_CMD_START);

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
		status = (read_csr(dd, MISC_CFG_FW_CTRL)
			   & MISC_CFG_FW_CTRL_RSA_STATUS_SMASK)
			     >> MISC_CFG_FW_CTRL_RSA_STATUS_SHIFT;

		if (status == RSA_STATUS_IDLE) {
			/* should not happen */
			dd_dev_err(dd, "%s firmwre security bad idle state\n",
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
			dd_dev_err(dd, "%s firmware security time out\n", who);
			ret = -ETIMEDOUT;
			break;
		}

		msleep(20);
	}

	/*
	 * Arrive here on success or failure.  Clear all RSA engine
	 * errors.  All current errors will stick - the RSA logic is keeping
	 * error high.  All previous errors will clear - the RSA logic
	 * is not keeping the error high.
	 */
	write_csr(dd, MISC_ERR_CLEAR,
			MISC_ERR_STATUS_MISC_FW_AUTH_FAILED_ERR_SMASK
			| MISC_ERR_STATUS_MISC_KEY_MISMATCH_ERR_SMASK);
	/*
	 * All that is left are the current errors.  Print failure details,
	 * if any.
	 */
	reg = read_csr(dd, MISC_ERR_STATUS);
	if (ret) {
		if (reg & MISC_ERR_STATUS_MISC_FW_AUTH_FAILED_ERR_SMASK)
			dd_dev_err(dd, "%s firmware authorization failed\n",
				who);
		if (reg & MISC_ERR_STATUS_MISC_KEY_MISMATCH_ERR_SMASK)
			dd_dev_err(dd, "%s firmware key mismatch\n", who);
	}

	return ret;
}

static void load_security_variables(struct hfi_devdata *dd,
					struct firmware_details *fdet)
{
	if (!fw_validate)
		return;	/* nothing to do */

	/* Security variables a.  Write the modulus */
	write_rsa_data(dd, MISC_CFG_RSA_MODULUS, fdet->modulus, KEY_SIZE);
	/* Security variables b.  Write the r2 */
	write_rsa_data(dd, MISC_CFG_RSA_R2, fdet->r2, KEY_SIZE);
	/* Security variables c.  Write the mu */
	write_rsa_data(dd, MISC_CFG_RSA_MU, fdet->mu, MU_SIZE);
	/* Security variables d.  Write the header */
	write_streamed_rsa_data(dd, MISC_CFG_SHA_PRELOAD,
			(u8 *)fdet->css_header, sizeof(struct css_header));
}

/* return the 8051 firmware state */
static inline u32 get_firmware_state(struct hfi_devdata *dd)
{
	u64 reg = read_csr(dd, DC_DC8051_STS_CUR_STATE);

	return (reg >> DC_DC8051_STS_CUR_STATE_FIRMWARE_SHIFT)
				& DC_DC8051_STS_CUR_STATE_FIRMWARE_MASK;
}

/*
 * Wait until the firmware is up and ready to take host requests.
 * Return 0 on success, -ETIMEDOUT on timeout.
 */
int wait_fm_ready(struct hfi_devdata *dd, u32 mstimeout)
{
	unsigned long timeout;

	/* in the simulator, the fake 8051 is always ready */
	if (dd->icode == ICODE_FUNCTIONAL_SIMULATOR)
		return 0;

	timeout = msecs_to_jiffies(mstimeout) + jiffies;
	while (1) {
		if (get_firmware_state(dd) == 0xa0)	/* ready */
			return 0;
		if (time_after(jiffies, timeout))	/* timed out */
			return -ETIMEDOUT;
		usleep_range(1950, 2050); /* sleep 2ms-ish */
	}
}

/*
 * Load the 8051 firmware.
 */
static int load_8051_firmware(struct hfi_devdata *dd,
				struct firmware_details *fdet)
{
	u64 reg;
	int ret;
	u8 ver_a, ver_b;

	/*
	 * DC Reset sequence
	 * Load DC 8051 firmware
	 */
	/*
	 * DC reset step 1: Reset DC8051
	 */
	reg = DC_DC8051_CFG_RST_M8051W_SMASK
		| DC_DC8051_CFG_RST_CRAM_SMASK
		| DC_DC8051_CFG_RST_DRAM_SMASK
		| DC_DC8051_CFG_RST_IRAM_SMASK
		| DC_DC8051_CFG_RST_SFR_SMASK;
	write_csr(dd, DC_DC8051_CFG_RST, reg);

	/*
	 * DC reset step 2 (optional): Load 8051 data memory with link
	 * configuration
	 */

	/*
	 * DC reset step 3: Load DC8051 firmware
	 */
	/* release all but the core reset */
	reg = DC_DC8051_CFG_RST_M8051W_SMASK;
	write_csr(dd, DC_DC8051_CFG_RST, reg);

	/* Firmware load step 1 */
	load_security_variables(dd, fdet);

	/*
	 * Firmware load step 2.  Clear MISC_CFG_FW_CTRL.FW_8051_LOADED
	 */
	write_csr(dd, MISC_CFG_FW_CTRL,
			(fw_validate ? 0 :
			    MISC_CFG_FW_CTRL_DISABLE_VALIDATION_SMASK));

	/* Firmware load steps 3-5 */
	ret = write_8051(dd, 1/*code*/, 0, fdet->firmware_ptr,
							fdet->firmware_len);
	if (ret)
		return ret;

	/*
	 * DC reset step 4. Host starts the DC8051 firmware
	 */
	/*
	 * Firmware load step 6.  Set MISC_CFG_FW_CTRL.FW_8051_LOADED
	 * Clear or set DISABLE_VALIDATION dependig on if we are validating.
	 */
	write_csr(dd, MISC_CFG_FW_CTRL,
			MISC_CFG_FW_CTRL_FW_8051_LOADED_SMASK |
			(fw_validate ? 0 :
			    MISC_CFG_FW_CTRL_DISABLE_VALIDATION_SMASK));

	/* Firmware load steps 7-10 */
	ret = run_rsa(dd, "8051", fdet->signature);
	if (ret)
		return ret;

	/* clear all reset bits, releasing the 8051 */
	write_csr(dd, DC_DC8051_CFG_RST, 0ull);

	/*
	 * DC reset step 5. Wait for firmware to be ready to accept host
	 * requests.
	 */
	ret = wait_fm_ready(dd, TIMEOUT_8051_START);
	if (ret) { /* timed out */
		dd_dev_err(dd, "8051 start timeout, current state 0x%x\n",
			get_firmware_state(dd));
		return -ETIMEDOUT;
	}

	read_misc_status(dd, &ver_a, &ver_b);
	dd_dev_info(dd, "8051 firmware version %d.%d\n",
		(int)ver_b, (int)ver_a);

	return 0;
}

/* SBus Master broadcast address */
#define SBUS_MASTER_BROADCAST 0xfd

/*
 * Write the SBus request register
 *
 * No need for masking - the arguments are sized exactly.
 */
void sbus_request(struct hfi_devdata *dd,
		u8 receiver_addr, u8 data_addr, u8 command, u32 data_in)
{
	write_csr(dd, ASIC_CFG_SBUS_REQUEST,
		((u64)data_in << ASIC_CFG_SBUS_REQUEST_DATA_IN_SHIFT)
		| ((u64)command << ASIC_CFG_SBUS_REQUEST_COMMAND_SHIFT)
		| ((u64)data_addr << ASIC_CFG_SBUS_REQUEST_DATA_ADDR_SHIFT)
		| ((u64)receiver_addr
			<< ASIC_CFG_SBUS_REQUEST_RECEIVER_ADDR_SHIFT));
}

/*
 * Turn off the SBus and fabric serdes spicos.
 *
 * + Must be called with Sbus fast mode turned on.
 * + Must be called after fabric serdes broadcast is set up.
 * + Must be called before the 8051 is loaded - assumes 8051 is not loaded
 *   when using MISC_CFG_FW_CTRL.
 */
static void turn_off_spicos(struct hfi_devdata *dd, int flags)
{
	/* only needed on A0 */
	if (!is_a0(dd))
		return;

	/*
	 * This step is not needed if not validating the firmware.
	 * In addition, don't modify MISC_CFG_FW_CTRL if not validating.
	 */
	if (!fw_validate)
		return;

	dd_dev_info(dd, "Turning off spicos:%s%s\n",
		flags & SPICO_SBUS ? " SBus" : "",
		flags & SPICO_FABRIC ? " fabric" : "");

	write_csr(dd, MISC_CFG_FW_CTRL,
			    MISC_CFG_FW_CTRL_DISABLE_VALIDATION_SMASK);
	/* disable SBus spico */
	if (flags & SPICO_SBUS)
		sbus_request(dd, SBUS_MASTER_BROADCAST, 0x01,
			WRITE_SBUS_RECEIVER, 0x00000040);

	/* disable the fabric serdes spicos */
	if (flags & SPICO_FABRIC)
		sbus_request(dd, fabric_serdes_broadcast[dd->hfi_id],
				0x07, WRITE_SBUS_RECEIVER, 0x00000000);
	write_csr(dd, MISC_CFG_FW_CTRL, 0);
}

/*
 *  Reset all of the fabric serdes for our HFI.
 */
void fabric_serdes_reset(struct hfi_devdata *dd)
{
	u8 ra;

	if (dd->icode != ICODE_RTL_SILICON) /* only for RTL */
		return;

	ra = fabric_serdes_broadcast[dd->hfi_id];

	acquire_hw_mutex(dd);
	set_sbus_fast_mode(dd);
	/* place SerDes in reset and disable SPICO */
	sbus_request(dd, ra, 0x07, WRITE_SBUS_RECEIVER, 0x00000011);
	/* wait 100 refclk cycles @ 156.25MHz => 640ns */
	udelay(1);
	/* remove SerDes reset */
	sbus_request(dd, ra, 0x07, WRITE_SBUS_RECEIVER, 0x00000010);
	/* turn SPICO enable on */
	sbus_request(dd, ra, 0x07, WRITE_SBUS_RECEIVER, 0x00000002);
	clear_sbus_fast_mode(dd);
	release_hw_mutex(dd);
}

/* Access to the SBus in this routine should probably be serialized */
int sbus_request_slow(struct hfi_devdata *dd,
		      u8 receiver_addr, u8 data_addr, u8 command, u32 data_in)
{
	u64 reg, count = 0;
	int ret = 0;

	sbus_request(dd, receiver_addr, data_addr, command, data_in);
	write_csr(dd, ASIC_CFG_SBUS_EXECUTE,
		  ASIC_CFG_SBUS_EXECUTE_EXECUTE_SMASK);
	/* Wait for both DONE and RCV_DATA_VALID to go high */
	reg = read_csr(dd, ASIC_STS_SBUS_RESULT);
	while (!((reg & ASIC_STS_SBUS_RESULT_DONE_SMASK) &&
		 (reg & ASIC_STS_SBUS_RESULT_RCV_DATA_VALID_SMASK))) {
		if (count++ >= SBUS_MAX_POLL_COUNT)
			/* We timed out waiting but proceed anyway */
			break;
		udelay(1);
		reg = read_csr(dd, ASIC_STS_SBUS_RESULT);
	}
	write_csr(dd, ASIC_CFG_SBUS_EXECUTE, 0);
	/* Wait for DONE to clear after EXECUTE is cleared */
	reg = read_csr(dd, ASIC_STS_SBUS_RESULT);
	while (reg & ASIC_STS_SBUS_RESULT_DONE_SMASK) {
		if (count++ >= SBUS_MAX_POLL_COUNT) {
			ret = -ETIME;
			break;
		}
		udelay(1);
		reg = read_csr(dd, ASIC_STS_SBUS_RESULT);
	}
	return ret;
}

static int load_fabric_serdes_firmware(struct hfi_devdata *dd,
					struct firmware_details *fdet)
{
	int i, err;
	const u8 ra = fabric_serdes_broadcast[dd->hfi_id]; /* receiver addr */

	dd_dev_info(dd, "Downloading fabric firmware\n");

	/* step 1: load security variables */
	load_security_variables(dd, fdet);
	/* step 2: place SerDes in reset and disable SPICO */
	sbus_request(dd, ra, 0x07, WRITE_SBUS_RECEIVER, 0x00000011);
	/* wait 100 refclk cycles @ 156.25MHz => 640ns */
	udelay(1);
	/* step 3:  remove SerDes reset */
	sbus_request(dd, ra, 0x07, WRITE_SBUS_RECEIVER, 0x00000010);
	/* step 4: assert IMEM override */
	sbus_request(dd, ra, 0x00, WRITE_SBUS_RECEIVER, 0x40000000);
	/* step 5: download SerDes machine code */
	for (i = 0; i < fdet->firmware_len; i += 4) {
		sbus_request(dd, ra, 0x0a, WRITE_SBUS_RECEIVER,
					*(u32 *)&fdet->firmware_ptr[i]);
	}
	/* step 6: IMEM override off */
	sbus_request(dd, ra, 0x00, WRITE_SBUS_RECEIVER, 0x00000000);
	/* step 7: turn ECC on */
	sbus_request(dd, ra, 0x0b, WRITE_SBUS_RECEIVER, 0x000c0000);

	/* steps 8-11: run the RSA engine */
	err = run_rsa(dd, "fabric serdes", fdet->signature);
	if (err)
		return err;

	/* step 12: turn SPICO enable on */
	sbus_request(dd, ra, 0x07, WRITE_SBUS_RECEIVER, 0x00000002);
	/* step 13: enable core hardware interrupts */
	sbus_request(dd, ra, 0x08, WRITE_SBUS_RECEIVER, 0x00000000);

	return 0;
}

static int load_sbus_firmware(struct hfi_devdata *dd,
				struct firmware_details *fdet)
{
	int i, err;
	const u8 ra = SBUS_MASTER_BROADCAST; /* receiver address */

	dd_dev_info(dd, "Downloading SBus firmware\n");

	/* step 1: load security variables */
	load_security_variables(dd, fdet);
	/* step 2: place SPICO into reset and enable off */
	sbus_request(dd, ra, 0x01, WRITE_SBUS_RECEIVER, 0x000000c0);
	/* step 3: remove reset, enable off, IMEM_CNTRL_EN on */
	sbus_request(dd, ra, 0x01, WRITE_SBUS_RECEIVER, 0x00000240);
	/* step 4: set starting IMEM address for burst download */
	sbus_request(dd, ra, 0x03, WRITE_SBUS_RECEIVER, 0x80000000);
	/* step 5: download the SBus Master machine code */
	for (i = 0; i < fdet->firmware_len; i += 4) {
		sbus_request(dd, ra, 0x14, WRITE_SBUS_RECEIVER,
					*(u32 *)&fdet->firmware_ptr[i]);
	}
	/* step 6: set IMEM_CNTL_EN off */
	sbus_request(dd, ra, 0x01, WRITE_SBUS_RECEIVER, 0x00000040);
	/* step 7: turn ECC on */
	sbus_request(dd, ra, 0x16, WRITE_SBUS_RECEIVER, 0x000c0000);

	/* steps 8-11: run the RSA engine */
	err = run_rsa(dd, "SBus", fdet->signature);
	if (err)
		return err;

	/* step 12: set SPICO_ENABLE on */
	sbus_request(dd, ra, 0x01, WRITE_SBUS_RECEIVER, 0x00000140);

	return 0;
}

static int load_pcie_serdes_firmware(struct hfi_devdata *dd,
					struct firmware_details *fdet)
{
	int i, err;
	const u8 ra = SBUS_MASTER_BROADCAST; /* receiver address */

	dd_dev_info(dd, "Downloading PCIe firmware\n");

	/* step 1: load security variables */
	load_security_variables(dd, fdet);
	/* step 2: assert single step (halts the SBus Master spico) */
	sbus_request(dd, ra, 0x05, WRITE_SBUS_RECEIVER, 0x00000001);
	/* step 3: enable XDMEM access */
	sbus_request(dd, ra, 0x01, WRITE_SBUS_RECEIVER, 0x00000d40);
	/* step 4: load firmware into SBus Master XDMEM */
	/* NOTE: the dmem address, write_en, and wdata are all pre-packed,
	   we only need to pick up the bytes and write them */
	for (i = 0; i < fdet->firmware_len; i += 4) {
		sbus_request(dd, ra, 0x04, WRITE_SBUS_RECEIVER,
					*(u32 *)&fdet->firmware_ptr[i]);
	}
	/* step 5: disable XDMEM access */
	sbus_request(dd, ra, 0x01, WRITE_SBUS_RECEIVER, 0x00000140);
	/* step 6: allow SBus Spico to run */
	sbus_request(dd, ra, 0x05, WRITE_SBUS_RECEIVER, 0x00000000);

	/* steps 7-10: run RSA */
	err = run_rsa(dd, "PCIe serdes", fdet->signature);
	if (err)
		return err;

	/* step 11: firmware is available to be swapped */

	return 0;
}

/*
 * Set the given broadcast values on the given list of devices.
 */
static void set_serdes_broadcast(struct hfi_devdata *dd, u8 bg1, u8 bg2,
					const u8 *addrs, int count)
{
	while (--count >= 0) {
		/*
		 * Set BROADCAST_GROUP_1 and BROADCAST_GROUP_2, leave
		 * defaults for everything else.  Do not read-modify-write,
		 * per instruction from the manufacturer.
		 *
		 * Register 0xfd:
		 *	bits    what
		 *	-----	---------------------------------
		 *	  0	IGNORE_BROADCAST  (default 0)
		 *	11:4	BROADCAST_GROUP_1 (default 0xff)
		 *	23:16	BROADCAST_GROUP_2 (default 0xff)
		 */
		sbus_request(dd, addrs[count], 0xfd, WRITE_SBUS_RECEIVER,
				(u32)bg1 << 4 | (u32)bg2 << 16);
	}
}

int acquire_hw_mutex(struct hfi_devdata *dd)
{
	unsigned long timeout;
	u8 mask = 1 << dd->hfi_id;
	u8 user;

	timeout = msecs_to_jiffies(HM_TIMEOUT) + jiffies;
	while (1) {
		write_csr(dd, ASIC_CFG_MUTEX, mask);
		user = (u8)read_csr(dd, ASIC_CFG_MUTEX);
		if (user == mask)
			return 0; /* success */
		if (time_after(jiffies, timeout))
			break; /* timed out */
		msleep(20);
	}

	/* timed out */
	/* alternate: break the mutex and continue */
	dd_dev_err(dd,
		"Unable to acquire hardware mutex, mutex mask %u, my mask %u\n",
		(u32)user, (u32)mask);

	return -EBUSY;
}

void release_hw_mutex(struct hfi_devdata *dd)
{
	write_csr(dd, ASIC_CFG_MUTEX, 0);
}

void set_sbus_fast_mode(struct hfi_devdata *dd)
{
	write_csr(dd, ASIC_CFG_SBUS_EXECUTE,
				ASIC_CFG_SBUS_EXECUTE_FAST_MODE_SMASK);
}

void clear_sbus_fast_mode(struct hfi_devdata *dd)
{
	u64 reg, count = 0;

	reg = read_csr(dd, ASIC_STS_SBUS_COUNTERS);
	while (SBUS_COUNTER(reg, EXECUTE) !=
	       SBUS_COUNTER(reg, RCV_DATA_VALID)) {
		if (count++ >= SBUS_MAX_POLL_COUNT)
			break;
		udelay(1);
		reg = read_csr(dd, ASIC_STS_SBUS_COUNTERS);
	}
	write_csr(dd, ASIC_CFG_SBUS_EXECUTE, 0);
}

int load_firmware(struct hfi_devdata *dd)
{
	int ret;

	if (fw_sbus_load || fw_fabric_serdes_load) {
		ret = acquire_hw_mutex(dd);
		if (ret)
			return ret;

		set_sbus_fast_mode(dd);

		/*
		 * The SBus contains part of the fabric firmware and so must
		 * also be downloaded.
		 */
		if (fw_sbus_load) {
			turn_off_spicos(dd, SPICO_SBUS);
			ret = load_sbus_firmware(dd, &fw_sbus);
			if (ret)
				goto clear;
		}

		if (fw_fabric_serdes_load) {
			set_serdes_broadcast(dd, all_fabric_serdes_broadcast,
					fabric_serdes_broadcast[dd->hfi_id],
					fabric_serdes_addrs[dd->hfi_id],
					NUM_FABRIC_SERDES);
			turn_off_spicos(dd, SPICO_FABRIC);
			ret = load_fabric_serdes_firmware(dd, &fw_fabric);
		}

clear:
		clear_sbus_fast_mode(dd);
		release_hw_mutex(dd);
		if (ret)
			return ret;
	}

	if (fw_8051_load) {
		ret = load_8051_firmware(dd, &fw_8051);
		if (ret)
			return ret;
	}

	return 0;
}

int hfi1_firmware_init(struct hfi_devdata *dd)
{
	int ret;

	/* we do not expect more than 2 HFIs */
	BUG_ON(dd->hfi_id >= 2);

	/* only RTL can use these */
	if (dd->icode != ICODE_RTL_SILICON) {
		fw_fabric_serdes_load = 0;
		fw_pcie_serdes_load = 0;
		fw_sbus_load = 0;
	}

	if (!fw_8051_name) {
		if (dd->icode == ICODE_RTL_SILICON)
			fw_8051_name = DEFAULT_FW_8051_NAME_ASIC;
		else
			fw_8051_name = DEFAULT_FW_8051_NAME_FPGA;
	}
	if (!fw_fabric_serdes_name)
		fw_fabric_serdes_name = DEFAULT_FW_FABRIC_NAME;
	if (!fw_sbus_name)
		fw_sbus_name = DEFAULT_FW_SBUS_NAME;
	if (!fw_pcie_serdes_name)
		fw_pcie_serdes_name = DEFAULT_FW_PCIE_NAME;

	ret = obtain_firmware(dd);
	if (ret)
		return ret;

	/*
	 * Expect that we enter this routine with MISC_CFG_FW_CTRL reset:
	 *	- FW_8051_LOADED clear
	 *	- DISABLE_VALIDATION clear
	 */
	if (!fw_validate)
		write_csr(dd, MISC_CFG_FW_CTRL,
			    MISC_CFG_FW_CTRL_DISABLE_VALIDATION_SMASK);

	return 0;
}

/*
 * Download the firmware needed for the Gen3 PCIe SerDes.  An update
 * to the SBus firmware is needed before updating the PCIe firmware.
 *
 * Note: caller must be holding the HW mutex.
 */
int load_pcie_firmware(struct hfi_devdata *dd)
{
	int ret = 0;

	/* both firmware loads below use the SBus */
	set_sbus_fast_mode(dd);

	if (fw_sbus_load) {
		turn_off_spicos(dd, SPICO_SBUS);
		ret = load_sbus_firmware(dd, &fw_sbus);
		if (ret)
			goto done;
		fw_sbus_load = 0;	/* only load it once */
	}

	if (fw_pcie_serdes_load) {
		dd_dev_info(dd, "Setting PCIe SerDes broadcast\n");
		set_serdes_broadcast(dd, all_pcie_serdes_broadcast,
					pcie_serdes_broadcast[dd->hfi_id],
					pcie_serdes_addrs[dd->hfi_id],
					NUM_PCIE_SERDES);
		ret = load_pcie_serdes_firmware(dd, &fw_pcie);
		if (ret)
			goto done;
	}

done:
	clear_sbus_fast_mode(dd);

	return ret;
}

/*
 * Read the GUID from the hardware, store it in dd.
 */
void read_guid(struct hfi_devdata *dd)
{
	dd->base_guid = cpu_to_be64(read_csr(dd, DC_DC8051_CFG_LOCAL_GUID));
	dd_dev_info(dd, "GUID %llx",
		(unsigned long long)be64_to_cpu(dd->base_guid));
}
