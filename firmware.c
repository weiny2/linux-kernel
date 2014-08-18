/*
 * Copyright (c) 2013 Intel Corporation.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <linux/firmware.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/delay.h>

#include "hfi.h"

static uint fw_8051_load;
module_param_named(fw_8051_load, fw_8051_load, uint, S_IRUGO);
MODULE_PARM_DESC(fw_8051_load, "Load the 8051 firmware");

static uint fw_fabric_serdes_load;
module_param_named(fw_fabric_serdes_load, fw_fabric_serdes_load, uint, S_IRUGO);
MODULE_PARM_DESC(fw_fabric_serdes_load, "Load the fabric SerDes firmware");

static uint fw_pcie_serdes_load;
module_param_named(fw_pcie_serdes_load, fw_pcie_serdes_load, uint, S_IRUGO);
MODULE_PARM_DESC(fw_pcie_serdes_load, "Load the PCIe SerDes firmware");

static uint fw_sbus_load;
module_param_named(fw_sbus_load, fw_sbus_load, uint, S_IRUGO);
MODULE_PARM_DESC(fw_sbus_load, "Load the SBUS firmware");

static uint fw_validate;
module_param_named(fw_validate, fw_validate, uint, S_IRUGO);
MODULE_PARM_DESC(fw_validate, "Perform firmware validation");

#define DEFAULT_FW_8051_NAME "hfi_dc8051.bin"
#define DEFAULT_FW_FABRIC_NAME "hfi_fabric_serdes.bin"
#define DEFAULT_FW_SBUS_NAME "hfi_sbus_master.bin"
#define DEFAULT_FW_PCIE_NAME "hfi_pcie_serdes.bin"

static char *fw_8051_name = DEFAULT_FW_8051_NAME;
module_param_named(fw_8051_name, fw_8051_name, charp, S_IRUGO);
MODULE_PARM_DESC(fw_8051_name, "8051 firmware name");

static char *fw_fabric_serdes_name = DEFAULT_FW_FABRIC_NAME;
module_param_named(fw_fabric_serdes_name, fw_fabric_serdes_name, charp, S_IRUGO);
MODULE_PARM_DESC(fw_fabric_serdes_name, "Fabric SerDes firmware name");

static char *fw_sbus_name = DEFAULT_FW_SBUS_NAME;
module_param_named(fw_sbus_name, fw_sbus_name, charp, S_IRUGO);
MODULE_PARM_DESC(fw_sbus_name, "SBUS firmware name");

static char *fw_pcie_serdes_name = DEFAULT_FW_PCIE_NAME;
module_param_named(fw_pcie_serdes_name, fw_pcie_serdes_name, charp, S_IRUGO);
MODULE_PARM_DESC(fw_pcie_serdes_name, "PCIe SerDes firmware name");

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
	u8 r2[KEY_SIZE];
	u8 mu[MU_SIZE];
	u8 firmware[];
};

struct firmware_details {
	/* linux core piece */
	const struct firmware *fw;

	struct firmware_file *firmware;
	u8 *firmware_ptr;		/* pointer to binary data */
	u32 firmware_len;		/* length in bytes */
	struct firmware_file dummy_header;
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

/* security block commands */
#define RSA_CMD_INIT  0x1
#define RSA_CMD_START 0x2

/* security block status */
#define RSA_STATUS_IDLE   0x0
#define RSA_STATUS_ACTIVE 0x1
#define RSA_STATUS_DONE   0x2
#define RSA_STATUS_FAILED 0x3

/* firmware download timeouts, in ms */
#define FW_TIMEOUT_8051		 10 /* ms */
#define FW_TIMEOUT_FABRIC_SERDES 10 /* ms */
#define FW_TIMEOUT_SBUS		 10 /* ms */
#define FW_TIMEOUT_PCIE_SERDES	 10 /* ms */

/* 8051 start timeout, in ms */
#define TIMEOUT_8051_START 10 /* ms */

/* hardware mutex timeout, in ms */
#define HM_TIMEOUT 20 /* ms */

/* 8051 memory access timout, in us */
#define DC8051_ACCESS_TIMEOUT 100 /* us */

static int print_css_header = 1;	/* TODO: hook to verbosity level */

/* the number of SerDes on the SBUS */
#define NUM_FABRIC_SERDES 4
#define NUM_PCIE_SERDES 16

/* SBUS fabric SerDes addresses, one set per HFI */
static const u8 fabric_serdes_addrs[2][NUM_FABRIC_SERDES] = {
	{ 0x01, 0x02, 0x03, 0x04 },
	{ 0x28, 0x29, 0x2a, 0x2b }
};

/* SBUS PCIe SerDes addresses, one set per HFI */
static const u8 pcie_serdes_addrs[2][NUM_PCIE_SERDES] = {
	{ 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x14, 0x16,
	  0x18, 0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x24, 0x26 },
	{ 0x2f, 0x31, 0x33, 0x35, 0x37, 0x39, 0x3b, 0x3d,
	  0x3f, 0x41, 0x43, 0x45, 0x47, 0x49, 0x4b, 0x4d }
};

/* SBUS fabric SerDes broadcast addresses, one per HFI */
static const u8 fabric_serdes_broadcast[2] = { 0xe4, 0xe5 };
static const u8 all_fabric_serdes_broadcast = 0xe1;

/* SBUS PCIe SerDes broadcast addresses, one per HFI */
static const u8 pcie_serdes_broadcast[2] = { 0xe2, 0xe3 };
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
 * Request the firmware from the system.  Extract the pieces and fill in
 * fdet.  If succsessful, the caller will need to call dispose_one_firmware().
 * Returns 0 on success, -ERRNO on error.
 */
static int obtain_one_firmware(struct hfi_devdata *dd, const char *name,
				struct firmware_details *fdet)
{
	struct css_header *css;
	u32 inserted;
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
	if (print_css_header) {
		dd_dev_info(dd, "Firmware %s details:\n", name);
		dd_dev_info(dd, "file size: 0x%lx bytes\n", fdet->fw->size);
		dd_dev_info(dd, "CSS structure:\n");
		dd_dev_info(dd, "  module_type    0x%x\n", css->module_type);
		dd_dev_info(dd, "  header_len     0x%03x (0x%03x bytes)",
			css->header_len, 4*css->header_len);
		dd_dev_info(dd, "  header_version 0x%x\n", css->header_version);
		dd_dev_info(dd, "  module_id      0x%x\n", css->module_id);
		dd_dev_info(dd, "  module_vendor  0x%x\n", css->module_vendor);
		dd_dev_info(dd, "  date           0x%x\n", css->date);
		dd_dev_info(dd, "  size           0x%03x (0x%03x bytes)\n",
			css->size, 4*css->size);
		dd_dev_info(dd, "  key_size       0x%03x (0x%03x bytes)\n",
			css->key_size, 4*css->key_size);
		dd_dev_info(dd, "  modulus_size   0x%03x (0x%03x bytes)\n",
			css->modulus_size, 4*css->modulus_size);
		dd_dev_info(dd, "  exponent_size  0x%03x (0x%03x bytes)\n",
			css->exponent_size, 4*css->exponent_size);
		if (fdet->fw->size >= sizeof(struct firmware_file))
			dd_dev_info(dd, "firmware size: 0x%lx bytes\n",
				fdet->fw->size - sizeof(struct firmware_file));
		else
			dd_dev_info(dd, "firmware size: ? header too long\n");
	}

	if (css->module_vendor != CSS_MODULE_VENDOR) {
		/* assume this is a raw binary, with no CSS header */
		dd_dev_info(dd,
			"Invalid module vendor for \"%s\"- assuming raw binary, turning off validation",
			name);
		fw_validate = 0;
		/* assign a dummy header in case we go down an incorect path */
		fdet->firmware = &fdet->dummy_header;
		fdet->firmware_ptr = (u8 *)fdet->fw->data;
		fdet->firmware_len = fdet->fw->size;
		goto done;
	}

	fdet->firmware = (struct firmware_file *)fdet->fw->data;

	/*
	 * The r2 and mu fields were inserted after the CSS header was
	 * generated - it does not know about them.  When checking
	 * header lengths, adjust some sizes by the bytes inserted.
	 */
	inserted = sizeof(fdet->firmware->r2) + sizeof(fdet->firmware->mu);

	/* verify CSS header fields (most sizes are in DW, so add /4) */
	if (invalid_header(dd, "module_type", css->module_type, CSS_MODULE_TYPE)
			|| invalid_header(dd, "header_len", css->header_len,
					(sizeof(struct firmware_file)
						- inserted)/4)
			|| invalid_header(dd, "header_version",
					css->header_version, CSS_HEADER_VERSION)
			|| invalid_header(dd, "module_vendor",
					css->module_vendor, CSS_MODULE_VENDOR)
			|| invalid_header(dd, "size", css->size,
					((u32)fdet->fw->size-inserted)/4)
			|| invalid_header(dd, "key_size",
					css->key_size, KEY_SIZE/4)
			|| invalid_header(dd, "modulus_size",
					css->modulus_size, KEY_SIZE/4)
			|| invalid_header(dd, "exponent_size",
					css->exponent_size, EXPONENT_SIZE/4)) {
		ret = -EINVAL;
		goto done;
	}

	/* make sure we have some payload */
	if (sizeof(struct firmware_file) >= fdet->fw->size) {
		dd_dev_err(dd,
			"firmware \"%s\", size %ld, must be larger than %ld bytes\n",
			name, fdet->fw->size, sizeof(struct firmware_file));
		ret = -EINVAL;
		goto done;
	}

	fdet->firmware_ptr = fdet->firmware->firmware;
	fdet->firmware_len = fdet->fw->size - sizeof(struct firmware_file);

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
 * Write a block of data to a given array CSR.
 */
static void write_rsa_data(struct hfi_devdata *dd, int what,
				const u8 *data, int nbytes)
{
	int qw_size = nbytes/64;
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
 * Download the signature and start the RSA mechanism.  Wait for ms_timeout
 * before giving up.
 */
static int run_rsa(struct hfi_devdata *dd, unsigned long ms_timeout,
			const char *who, const u8 *signature)
{
	unsigned long timeout;
	u32 status;

	if (!fw_validate)
		return 0;	/* done with no error if not validating */

	/* write the signature */
	write_rsa_data(dd, WFR_MISC_CFG_RSA_SIGNATURE, signature, KEY_SIZE);

	/* init RSA */
	write_csr(dd, WFR_MISC_CFG_RSA_CMD, RSA_CMD_INIT);

	/* start RSA */
	write_csr(dd, WFR_MISC_CFG_RSA_CMD, RSA_CMD_START);

	/* look for result */
	timeout = msecs_to_jiffies(ms_timeout) + jiffies;
	while (1) {
		status = (read_csr(dd, WFR_MISC_CFG_FW_CTRL)
			   & WFR_MISC_CFG_FW_CTRL_RSA_STATUS_SMASK)
			     >> WFR_MISC_CFG_FW_CTRL_RSA_STATUS_SHIFT;

		switch (status) {
		case RSA_STATUS_IDLE:
			/* should not happen */
			dd_dev_err(dd, "%s firmwre security bad idle state\n",
				who);
			return -EINVAL;
		case RSA_STATUS_ACTIVE:
			/* still working */
			break;
		case RSA_STATUS_DONE:
			/* finished successfully */
			return 0;
		case RSA_STATUS_FAILED:
			/* finished unsuccessfully */
			dd_dev_err(dd, "%s firmwre security failure\n", who);
			return -EINVAL;
		};

		if (time_after(jiffies, timeout))
			break; /* timed out */
		msleep(1);
	}

	/* timed out */
	dd_dev_err(dd, "%s firmware security timeout, current status 0x%x\n",
		who, status);
	return -ETIMEDOUT;
}

static void load_security_variables(struct hfi_devdata *dd,
					struct firmware_details *fdet)
{
	if (!fw_validate)
		return;	/* nothing to do */

	/* Security variables a.  Write the modulus */
	write_rsa_data(dd, WFR_MISC_CFG_RSA_MODULUS,
			fdet->firmware->modulus, KEY_SIZE);
	/* Security variables b.  Write the r2 */
	write_rsa_data(dd, WFR_MISC_CFG_RSA_R2, fdet->firmware->r2, KEY_SIZE);
	/* Security variables c.  Write the mu */
	write_rsa_data(dd, WFR_MISC_CFG_RSA_MU, fdet->firmware->mu, MU_SIZE);
/* TODO: HAS 0.76 */
#ifdef WFR_MISC_CFG_SHA_PRELOAD
	/* Security variables d.  Write the header */
	write_rsa_data(dd, WFR_MISC_CFG_SHA_PRELOAD,
			(u8 *)&fdet->firmware->css_header,
			sizeof(struct css_header));
#endif
}

/* return the 8051 firmware statte */
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

	timeout = msecs_to_jiffies(mstimeout) + jiffies;
	while (1) {
		if (get_firmware_state(dd) == 0xa0)	/* ready */
			return 0;
		if (time_after(jiffies, timeout))	/* timed out */
			return -ETIMEDOUT;
		usleep_range(1950, 2050); /* sleep 2ms-ish */
	};
}

/*
 * Load the 8051 firmware.
 */
static int load_8051_firmware(struct hfi_devdata *dd,
				struct firmware_details *fdet)
{
	u64 reg;
	int ret;

	/*
	 * DC Reset sequence as described in the DC HAS, steps 1-5.
	 * Load DC 8051 firmware as described in the WFR HAS, steps 1-10.
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
	 * Assumes we are entering this routine with MISC_CFG_FW_CTRL
	 * reset.  This means that MISC_CFG_FW_CTRL.FW_8051_LOADED is
	 * alreay clear and we have nothing to do.
	 */

	/* Firmware load steps 3-5 */
	ret = write_8051(dd, 1/*code*/, 0, fdet->firmware_ptr,
							fdet->firmware_len);
	if (ret)
		return ret;

	/* TODO: guard with verbosity level */
	dd_dev_info(dd, "8051 firmware download stats: %u writes",
		(fdet->firmware_len+7)/8);

	/*
	 * DC reset step 4. Host starts the DC8051 firmware
	 */
	/*
	 * Firmware load step 6.  Set MISC_CFG_FW_CTRL.FW_8051_LOADED
	 * Clear or set DISABLE_VALIDATION dependig on if we are validating.
	 */
	write_csr(dd, WFR_MISC_CFG_FW_CTRL,
			WFR_MISC_CFG_FW_CTRL_FW_8051_LOADED_SMASK |
			(fw_validate ? 0 :
			    WFR_MISC_CFG_FW_CTRL_DISABLE_VALIDATION_SMASK));

	/* Firmware load steps 7-10 */
	ret = run_rsa(dd, FW_TIMEOUT_8051, "8051", fdet->firmware->signature);
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
		/*
		 * TODO: the functional simulator stopped doing this
		 * correctly in the v28-v34 timeframe.
		 */
		if (dd->icode == WFR_ICODE_FUNCTIONAL_SIMULATOR) {
			dd_dev_info(dd, "8051 start timed out (ignored)\n");
			return 0;
		}

		dd_dev_err(dd, "8051 start timeout, current state 0x%x\n",
			get_firmware_state(dd));
		return -ETIMEDOUT;
	}

	return 0;
}

/* SBUS Master broadcast address */
#define SBUS_MASTER_BROADCAST 0xfd

/* SBUS commands */
#define WRITE_SBUS_RECEIVER 0x21

/*
 * Write the SBUS request register
 *
 * No need for masking - the arguments are sized exactly.
 */
static inline void sbus_request(struct hfi_devdata *dd,
		u8 receiver_addr, u8 data_addr, u8 command, u32 data_in)
{
	write_csr(dd, WFR_ASIC_CFG_SBUS_REQUEST,
		((u64)data_in << WFR_ASIC_CFG_SBUS_REQUEST_DATA_IN_SHIFT)
		| ((u64)command << WFR_ASIC_CFG_SBUS_REQUEST_COMMAND_SHIFT)
		| ((u64)data_addr << WFR_ASIC_CFG_SBUS_REQUEST_DATA_ADDR_SHIFT)
		| ((u64)receiver_addr
			<< WFR_ASIC_CFG_SBUS_REQUEST_RECEIVER_ADDR_SHIFT));
}

static int load_fabric_serdes_firmware(struct hfi_devdata *dd,
					struct firmware_details *fdet)
{
	int i, err;
	u8 ra;				/* receiver address */

	ra = fabric_serdes_broadcast[dd->hfi_id];

	/* step 1: load security variables */
	load_security_variables(dd, fdet);
	/* step 2: place SerDes in reset and disable SPICO */
	sbus_request(dd, ra, 0x07, WRITE_SBUS_RECEIVER, 0x00000011);
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
	err = run_rsa(dd, FW_TIMEOUT_FABRIC_SERDES, "fabric serdes",
						fdet->firmware->signature);
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

	/* step 1: load security variables */
	load_security_variables(dd, fdet);
	/* step 2: place SPICO into reset and enable off */
	sbus_request(dd, ra, 0x01, WRITE_SBUS_RECEIVER, 0x000000c0);
	/* step 3: remove reset, enable off, IMEM_CNTRL_EN on */
	sbus_request(dd, ra, 0x01, WRITE_SBUS_RECEIVER, 0x00000240);
	/* step 4: set starting IMEM address for burst download */
	sbus_request(dd, ra, 0x03, WRITE_SBUS_RECEIVER, 0x80000000);
	/* step 5: download the SBUS Master machine code */
	for (i = 0; i < fdet->firmware_len; i += 4) {
		sbus_request(dd, ra, 0x14, WRITE_SBUS_RECEIVER,
					*(u32 *)&fdet->firmware_ptr[i]);
	}
	/* step 6: set IMEM_CNTL_EN off */
	sbus_request(dd, ra, 0x01, WRITE_SBUS_RECEIVER, 0x00000040);
	/* step 7: turn ECC on */
	sbus_request(dd, ra, 0x16, WRITE_SBUS_RECEIVER, 0x000c0000);

	/* steps 8-11: run the RSA engine */
	err = run_rsa(dd, FW_TIMEOUT_SBUS, "SBUS", fdet->firmware->signature);
	if (err)
		return err;

	/* step 12: set SPICO_ENABLE on */
	sbus_request(dd, ra, 0x01, WRITE_SBUS_RECEIVER, 0x00000140);

	return 0;
}

static int load_pcie_serdes_firmware(struct hfi_devdata *dd,
					struct firmware_details *fdet)
{
	u64 reg;
	int i, err;
	const u8 ra = SBUS_MASTER_BROADCAST; /* receiver address */

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
	err = run_rsa(dd, FW_TIMEOUT_PCIE_SERDES, "PCIe serdes",
						fdet->firmware->signature);
	if (err)
		return err;

	/* step 11: trigger the gasket block */
	reg = (((u64)1 << dd->hfi_id)
			<< WFR_ASIC_PCIE_SD_HOST_CMD_INTRPT_CMD_SHIFT)
		| ((u64)pcie_serdes_broadcast[dd->hfi_id]
			<< WFR_ASIC_PCIE_SD_HOST_CMD_SBUS_RCVR_ADDR_SHIFT);
	write_csr(dd, WFR_ASIC_PCIE_SD_HOST_CMD, reg);


/*
FIXME: Once we swap the PCIe firmware, does that immediately
start gen3 negotiations and/or interrupt the link?

If so, we can't touch the chip until the parent says it has renegotiated
the link. (code below)

If not, then we'll need to manually trigger it.
*/
#if 0
	err = wait_for_parent_link_up(dd);
	if (err)
		return err;

/* PCIE firmware download status */
#define WFR_PCIE_FW_DL_STATUS_SUCCESS 0x1

	/* check the status - if we can read the CSR, its done, either way */
	reg = read_csr(dd, WFR_ASIC_PCIE_SD_HOST_STATUS);
	if (reg == ~0ull) {	/* PCIe read failed/timeout */
		do some thing drastic here
	}
	status = (reg >> WFR_ASIC_PCIE_SD_HOST_STATUS_FW_DNLD_STS_SHIFT)
				& WFR_ASIC_PCIE_SD_HOST_STATUS_FW_DNLD_STS_MASK;
	if (status != WFR_PCIE_FW_STATUS_SUCCESS)
		return -EFAIL;
	err = ((reg >> WFR_ASIC_PCIE_SD_HOST_STATUS_FW_DNLD_ERR_SHIFT)
		& WFR_ASIC_PCIE_SD_HOST_STATUS_FW_DNLD_ERR_MASK)
#endif

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

static int acquire_hw_mutex(struct hfi_devdata *dd)
{
	unsigned long timeout;
	u8 mask = 1 << dd->hfi_id;
	u8 user;

	timeout = msecs_to_jiffies(HM_TIMEOUT) + jiffies;
	while (1) {
		write_csr(dd, WFR_ASIC_CFG_MUTEX, mask);
		user = (u8)read_csr(dd, WFR_ASIC_CFG_MUTEX);
		if (user == mask)
			return 0; /* success */
		if (time_after(jiffies, timeout))
			break; /* timed out */
		msleep(1);
	}

	/* timed out */
	/* alternate: break the mutex and continue */
	dd_dev_err(dd,
		"Unable to acquire hardware mutex, mutex mask %u, my mask %u\n",
		(u32)user, (u32)mask);

	return -EBUSY;
}

static void release_hw_mutex(struct hfi_devdata *dd)
{
	write_csr(dd, WFR_ASIC_CFG_MUTEX, 0);
}

static void set_sbus_fast_mode(struct hfi_devdata *dd)
{
	write_csr(dd, WFR_ASIC_CFG_SBUS_EXECUTE,
				WFR_ASIC_CFG_SBUS_EXECUTE_FAST_MODE_SMASK);
}

int load_firmware(struct hfi_devdata *dd)
{
	int ret;

	/* we do not expect more than 2 HFIs */
	BUG_ON(dd->hfi_id >= 2);

	ret = obtain_firmware(dd);
	if (ret)
		return ret;

	ret = acquire_hw_mutex(dd);
	if (ret)
		return ret;

	/* set the SBUS master to fast mode, we do not need to set it back */
	set_sbus_fast_mode(dd);

	/*
	 * Expect that we enter this routine with MISC_CFG_FW_CTRL reset:
	 *	- FW_8051_LOADED clear
	 *	- DISABLE_VALIDATION clear
	 * Possibly set DISABLE_VALIDATION - EFUSE may override the disable.
	 */
	if (!fw_validate)
		write_csr(dd, WFR_MISC_CFG_FW_CTRL,
			    WFR_MISC_CFG_FW_CTRL_DISABLE_VALIDATION_SMASK);

	if (fw_sbus_load) {
		ret = load_sbus_firmware(dd, &fw_sbus);
		if (ret)
			goto done;
	}

	if (fw_pcie_serdes_load) {
		set_serdes_broadcast(dd, all_pcie_serdes_broadcast,
					pcie_serdes_broadcast[dd->hfi_id],
					pcie_serdes_addrs[dd->hfi_id],
					NUM_PCIE_SERDES);
		ret = load_pcie_serdes_firmware(dd, &fw_pcie);
		if (ret)
			goto done;
	}

	if (fw_fabric_serdes_load) {
		set_serdes_broadcast(dd, all_fabric_serdes_broadcast,
					fabric_serdes_broadcast[dd->hfi_id],
					fabric_serdes_addrs[dd->hfi_id],
					NUM_FABRIC_SERDES);
		ret = load_fabric_serdes_firmware(dd, &fw_fabric);
		if (ret)
			goto done;
	}

	if (fw_8051_load) {
		ret = load_8051_firmware(dd, &fw_8051);
		if (ret)
			goto done;
	}

done:
	release_hw_mutex(dd);

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
