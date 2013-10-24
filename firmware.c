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

#include "hfi.h"

static uint load_8051_fw   = 0;
module_param_named(load_8051_fw, load_8051_fw, uint, S_IRUGO);
MODULE_PARM_DESC(load_8051_fw, "Load the 8051 firmware");

static int load_fabric_fw = 0;
module_param_named(load_fabric_fw, load_fabric_fw, uint, S_IRUGO);
MODULE_PARM_DESC(load_fabric_fw, "Load the fabric SerDes firmware");

static int load_pcie_fw   = 0;
module_param_named(load_pcie_fw, load_pcie_fw, uint, S_IRUGO);
MODULE_PARM_DESC(load_pcie_fw, "Load the PCIe SerDes firmware");

static int load_sbus_fw   = 0;
module_param_named(load_sbus_fw, load_sbus_fw, uint, S_IRUGO);
MODULE_PARM_DESC(load_sbus_fw, "Load the SBUS firmware");


//FIXME: It is not clear if this is needed.  The DC HAS says that writes need
//to be verified, but the WFR says they are not.
//#define RAM_ACCESS_STATUS_NEEDED 1


#ifdef RAM_ACCESS_STATUS_NEEDED
/*
 * Write stats: Count the number of times we had to check to see if the
 * write completed.
 * - we know the number of writes by len/8, so we don't need to count it
 */
struct wstats {
	unsigned long nchecks;		/* total checks performed */
	unsigned long min_checks;	/* min_checks per op */
	unsigned long max_checks;	/* max checks per op */
};
#endif /* RAM_SCCESS_STATUS_NEEDED */

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

struct firmware_details {
	/* linux core piece */
	const struct firmware *fw;

	/* security pieces */
	const u8 *rsa_modulus;
	const u8 *rsa_exponent;
	const u8 *rsa_signature;
	const u8 *rsa_r2;		/* R^2 */
	const u8 *rsa_mu;

	/* payload pieces */
	const u8 *payload;		/* the data */
	u32 payload_len;		/* length in bytes */
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

#define FW_8051_NAME "testcss_dbg.bin"
#define FW_FABRIC_NAME "testcss_dbg.bin"
#define FW_SBUS_NAME "testcss_dbg.bin"
#define FW_PCIE_NAME "testcss_dbg.bin"


#define FW_SIGNATURE_DWORDS 64	/* size in DWORDS = 2048 bits */
#define FW_EXPONENT_DWORDS   1	/* size in DWORDS =   32 bits */

/* sizes in bytes */
#define FW_SIGNATURE_SIZE (FW_SIGNATURE_DWORDS*4)
#define FW_MU_SIZE 8

/* security block commands */
#define RSA_CMD_INIT  0x1
#define RSA_CMD_START 0x2

/* security block status */
#define RSA_STATUS_IDLE   0x0
#define RSA_STATUS_ACTIVE 0x1
#define RSA_STATUS_DONE   0x2
#define RSA_STATUS_FAILED 0x3

static int print_css_header = 1;	/* TODO: hook to verbosity level */
static int security_enabled = 0;	/* TODO make settable */
/* TODO: maybe "security_present" to skip writing to any security block CSR? */

/*
 * Write data or code to the 8051 code or data RAM.
 */
static int write_8051(struct hfi_devdata *dd, int code, u32 start,
					const u8 *data, u32 len
#ifdef RAM_ACCESS_STATUS_NEEDED
					, struct wstats *wstat
#endif /* RAM_SCCESS_STATUS_NEEDED */
					)
{
	u64 reg;
	u32 offset;
	int err = 0;
	int aligned;
#ifdef RAM_ACCESS_STATUS_NEEDED
	unsigned long timeout;
	unsigned long nreads;
	struct wstats local_wstat;

	/* init stats */
	if (!wstat)
		wstat = &local_wstat;
	memset(wstat, 0, sizeof(*wstat));
	wstat->min_checks = -1;
#endif

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
		int bytes = len-offset >= 8 ? 8: len-offset;
		if (bytes < 8) {
			reg = 0;
			memcpy(&reg, &data[offset], bytes);
		} else if (aligned) {
			reg = *(u64 *)&data[offset];
		} else {
			memcpy(&reg, &data[offset], 8);
		}
		write_csr(dd, DC_DC8051_CFG_RAM_ACCESS_WR_DATA, reg);
#ifdef RAM_ACCESS_STATUS_NEEDED
		/* wait for completion, timeout at 1/10 second */
		timeout = jiffies + (HZ/10 == 0 ? 1 : HZ/10);
		nreads = 0;
		do {
			if (time_after(jiffies, timeout)) {
				dd_dev_err(dd, "8501 write: write fails to complete at address 0x%x, giving up\n",
					start + offset);
				err = -ETIMEDOUT;
				goto done;
			}
			nreads++;
			reg = read_csr(dd, DC_DC8051_CFG_RAM_ACCESS_STATUS);
			/* done when ACCESS_COMPLETED goes to non-zero */
		} while ((reg & DC_DC8051_CFG_RAM_ACCESS_STATUS_ACCESS_COMPLETED_SMASK) == 0);
		wstat->nchecks += nreads;
		if (wstat->min_checks > nreads)
			wstat->min_checks = nreads;
		if (wstat->max_checks < nreads)
			wstat->max_checks = nreads;
#endif /* RAM_SCCESS_STATUS_NEEDED */
	}

#ifdef RAM_ACCESS_STATUS_NEEDED
done:
#endif /* RAM_SCCESS_STATUS_NEEDED */
	/* turn off write access, auto increment (also sets to data access) */
	write_csr(dd, DC_DC8051_CFG_RAM_ACCESS_CTRL, 0);
	write_csr(dd, DC_DC8051_CFG_RAM_ACCESS_SETUP, 0);

	return err;
}

/* return 0 if values match, non-zero and complain otherwise */
static int invalid_header(struct hfi_devdata *dd, const char *what,
				u32 actual, u32 expected)
{
	if (actual == expected)
		return 0;

	dd_dev_err(dd, "invalid firmware header field %s: expected 0x%x, actual 0x%x\n", what, expected, actual);
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
	const u8 *base;
	u32 prefix_dw;
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
		dd_dev_info(dd, "size: 0x%lx bytes\n", fdet->fw->size);
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
	}

	/*
	 * Expected firmware file format:
	 *
	 * What				Size (bytes)
	 * ---------------------------	----------------------
	 * CSS header			sizeof(struct css_header)
	 * public key modulus		FW_SIGNATURE_DWORDS*4
	 * public key exponent		FW_EXPONENT_DWORDS*4
	 * signature			FW_SIGNATURE_DWORDS*4
	 * payload			N
	 */
	prefix_dw = sizeof(struct css_header)/4		/* CSS header */
			+ FW_SIGNATURE_DWORDS		/* pub key modulus */
			+ FW_EXPONENT_DWORDS		/* pub key exponent */
			+ FW_SIGNATURE_DWORDS;		/* signature */

	/* verify CSS header fields */
	if (invalid_header(dd, "module_type", css->module_type, CSS_MODULE_TYPE)
			|| invalid_header(dd, "header_len",
					css->header_len, prefix_dw)
			|| invalid_header(dd, "header_version",
					css->header_version, CSS_HEADER_VERSION)
			|| invalid_header(dd, "module_vendor",
					css->module_vendor, CSS_MODULE_VENDOR)
			|| invalid_header(dd, "size",
					css->size*4, (u32)fdet->fw->size)
			|| invalid_header(dd, "key_size",
					css->key_size, FW_SIGNATURE_DWORDS)
			|| invalid_header(dd, "modulus_size",
					css->modulus_size, FW_SIGNATURE_DWORDS)
			|| invalid_header(dd, "exponent_size",
					css->exponent_size, FW_EXPONENT_DWORDS)) {
		ret = -EINVAL;
		goto done;
	}

	/* make sure we have some payload */
	if (prefix_dw * 4 >= fdet->fw->size) {
		dd_dev_err(dd, "firmware \"%s\", size %ld, must be larger than %d bytes\n", name, fdet->fw->size, prefix_dw*4);
		ret = -EINVAL;
		goto done;
	}

	/* extract the fields */
	base = fdet->fw->data + sizeof(struct css_header);
	fdet->rsa_modulus = base;
	base += FW_SIGNATURE_DWORDS * 4;
	fdet->rsa_exponent = base;
	base += FW_EXPONENT_DWORDS * 4;
	fdet->rsa_signature = base;
	base += FW_SIGNATURE_DWORDS * 4;
	fdet->payload = base;
	fdet->payload_len = fdet->fw->size - (base - fdet->fw->data);

	/* FIXME: need r2 signature and mu; use rsa_modulus for now */
	fdet->rsa_r2 = fdet->rsa_modulus;
	fdet->rsa_mu = fdet->rsa_modulus;

done:
	/* if returning an error, clean up after ourselves */
	if (ret)
		release_firmware(fdet->fw);
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

	if (load_8051_fw) {
		err = obtain_one_firmware(dd, FW_8051_NAME, &fw_8051);
		if (err)
			goto done;
	}

	if (load_fabric_fw) {
		err = obtain_one_firmware(dd, FW_FABRIC_NAME, &fw_fabric);
		if (err)
			goto done;
	}

	if (load_sbus_fw) {
		err = obtain_one_firmware(dd, FW_SBUS_NAME, &fw_sbus);
		if (err)
			goto done;
	}

	if (load_pcie_fw) {
		err = obtain_one_firmware(dd, FW_PCIE_NAME, &fw_pcie);
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
	/* TODO: Should we even bother witht his here? */
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
		for (i = 0; i < qw_size; i++, data+=8) {
			u64 value;
			memcpy(&value, data, 8);
			write_csr(dd, what + (8*i), value);
		}
	}
}

/*
 * Load the 8051 firmware.
 */
static int load_8051_firmware(struct hfi_devdata *dd,
				struct firmware_details *fdet)
{
	u64 reg, firmware, status;
#ifdef RAM_ACCESS_STATUS_NEEDED
	struct wstats wstats;
#endif /* RAM_SCCESS_STATUS_NEEDED */
	unsigned long timeout;
	int ret;

	/* TODO: Do I need to clear the bits in DC_DC8051_ERR_EN? */
	/* TODO: What are the error interrupt hookups from DC to WFR
	   sources?  (Mark Debbage has a sticky note on this in the
	   DC HAS).  ISSUE: Why is it "ERR_EN" but you need to _clear_
	   the bits to enable it?  It seems backward. */

	/*
	 * Reset sequence as described in the DC HAS.  This is the firmware
	 * load, steps 1-6.  Link bringup, steps 7-10, are in the link
	 * bringup function.
	 */
	/*
	 * DC reset step 1: Reset all
	 */
	reg = DC_DC8051_CFG_RST_M8051W_SMASK
		| DC_DC8051_CFG_RST_CRAM_SMASK
		| DC_DC8051_CFG_RST_DRAM_SMASK
		| DC_DC8051_CFG_RST_IRAM_SMASK
		| DC_DC8051_CFG_RST_SFR_SMASK;
	write_csr(dd, DC_DC8051_CFG_RST, reg);

	/*
	 * DC reset step 2: Load firmware
	 */
	/* release all but the core reset */
	reg = DC_DC8051_CFG_RST_M8051W_SMASK;
	write_csr(dd, DC_DC8051_CFG_RST, reg);

	if (security_enabled) {
		/* Security step 1a.  Write the modulus, 2048 bits - 32 QW */
		write_rsa_data(dd, WFR_MISC_CFG_RSA_MODULUS,
				fdet->rsa_modulus, FW_SIGNATURE_SIZE);
		/* Security step 1b.  Write the r2, 2048 bits - 32 QW */
		write_rsa_data(dd, WFR_MISC_CFG_RSA_R2,
				fdet->rsa_r2, FW_SIGNATURE_SIZE);
		/* Security step 1c.  Write the mu, 64 bits */
		write_rsa_data(dd, WFR_MISC_CFG_RSA_MU,
				fdet->rsa_mu, FW_MU_SIZE);

		/*
		 * Security step 2a.  Clear MISC_CFG_FW_CTRL.FW_8051_LOADED
		 * OK to clear MISC_CFG_FW_CTRL.DISABLE_VALIDATION - we're
		 * in the validation path
		 */
		write_csr(dd, WFR_MISC_CFG_FW_CTRL, 0);
	} else {
		/*
		 * Turn security checking off.  Only works if EFUSE
		 * settings allow it.
		 */
		write_csr(dd, WFR_MISC_CFG_FW_CTRL,
				WFR_MISC_CFG_FW_CTRL_DISABLE_VALIDATION_SMASK);
		
	}

	/* Security steps 2b-2d.  Load firmware */
	/* TODO: firmware start of 0? */
#ifdef RAM_ACCESS_STATUS_NEEDED
	ret = write_8051(dd, 1/*code*/, 0, fdet->payload, fdet->payload_len,
								&wstats);
#else
	ret = write_8051(dd, 1/*code*/, 0, fdet->payload, fdet->payload_len);
#endif /* RAM_SCCESS_STATUS_NEEDED */
	if (ret)
		return ret;
		

	/* TODO: guard with verbosity level */
	dd_dev_info(dd, "8051 firmware download stats:");
	dd_dev_info(dd, "  writes:         %u\n",  (fdet->payload_len+7)/8);
#ifdef RAM_ACCESS_STATUS_NEEDED
	dd_dev_info(dd, "  checks:         %ld\n", wstats.nchecks);
	dd_dev_info(dd, "  min check loop: %ld\n", wstats.min_checks);
	dd_dev_info(dd, "  max check loop: %ld\n", wstats.max_checks);
#endif /* RAM_SCCESS_STATUS_NEEDED */

	/*
	 * DC reset step 3. Load link configuration (optional)
	 */
	/* TODO TBD - where do we get these values? */

	/*
	 * DC reset step 4. Start the 8051
	 */
	/* clear all reset bits, releasing the 8051 */
	write_csr(dd, DC_DC8051_CFG_RST, 0ull);

	if (security_enabled) {
		/*
		 * Security step 2e.  Set MISC_CFG_FW_CTRL.FW_8051_LOADED
		 * OK to clear DISABLE_VALIDATION - we're in the validation
		 * path
		 */
		write_csr(dd, WFR_MISC_CFG_FW_CTRL,
				WFR_MISC_CFG_FW_CTRL_FW_8051_LOADED_SMASK);

		/* Security step 2f.  Write the signature, 2048 bits - 32 QW */
		write_rsa_data(dd, WFR_MISC_CFG_RSA_SIGNATURE,
				fdet->rsa_signature, FW_SIGNATURE_SIZE);

		/* Security step 2g.  Write MISC_CFG_RSA_CMD = 1 (INIT) */
		write_csr(dd, WFR_MISC_CFG_RSA_CMD, RSA_CMD_INIT);

		/* Security step 2h.  Write MISC_CFG_RSA_CMD = 2 (START) */
		write_csr(dd, WFR_MISC_CFG_RSA_CMD, RSA_CMD_START);

		/*
		 * Security step 2i.  Poll MISC_CFG_FW_CTRL.RSA_STATUS
		 *	- If ACTIVE: continue polling
		 *	- If DONE: validation successful, 8051 reset deasserted
		 *	  by hardware
		 *	- If FAILED: validation failed, 8051 reset remains
		 *	  held by hardware, (TBD - CCE error flag) set
		 */
		/* timeout at 1/10 second */
		timeout = jiffies + (HZ/10 == 0 ? 1 : HZ/10);
		status = 0;
		do {
			if (time_after(jiffies, timeout)) {
				dd_dev_err(dd, "8051 firmware security timeout, current status 0x%llx\n", status);
				return -ETIMEDOUT;
			}
			reg = read_csr(dd, WFR_MISC_CFG_FW_CTRL);
			status = (reg >> WFR_MISC_CFG_FW_CTRL_RSA_STATUS_SHIFT)
					& WFR_MISC_CFG_FW_CTRL_RSA_STATUS_MASK;
			if (status == RSA_STATUS_FAILED) {
				dd_dev_err(dd, "8051 firmware security failure\n");
				return -EINVAL;
			}
		} while (status != RSA_STATUS_DONE);
	}
	/*
	 * FIXME: Workaround for v15 simulator bug.  Remove this block
	 * when fixed.
	 *
	 * The v15 simulator always requires MISC_CFG_FW_CTRL.FW_8051_LOADED
	 * to be set to bring the 8051 out of rest.  Instead, it should
	 * depend on whether WFR_MISC_CFG_FW_CTRL.DISABLE_VALIDATION is
	 * set.
	 */
	else {
		write_csr(dd, WFR_MISC_CFG_FW_CTRL,
				WFR_MISC_CFG_FW_CTRL_FW_8051_LOADED_SMASK);
	}

	/*
	 * DC reset step 5. Wait for firmware to be ready to accept host
	 * requests.
	 */
	/* timeout at 1/10 second */
	timeout = jiffies + (HZ/10 == 0 ? 1 : HZ/10);
	firmware = 0;
	do {
		if (time_after(jiffies, timeout)) {
			dd_dev_err(dd, "8051 start timeout, current state 0x%llx\n", firmware);
			return -ETIMEDOUT;
		}
		reg = read_csr(dd, DC_DC8051_STS_CUR_STATE);
		firmware = (reg >> DC_DC8051_STS_CUR_STATE_FIRMWARE_SHIFT)
				& DC_DC8051_STS_CUR_STATE_FIRMWARE_MASK;
	} while (firmware != 0xa0);	/* ready for HOST request */

	return ret;
}

static int load_serdes_firmware(struct hfi_devdata *dd,
					struct firmware_details *fdet)
{
	//FIXME: implement
	return 0;
}

static int load_sbus_firmware(struct hfi_devdata *dd,
				struct firmware_details *fdet)
{
	//FIXME: implement
	return 0;
}

int load_firmware(struct hfi_devdata *dd)
{
	int ret;

	ret = obtain_firmware(dd);
	if (ret)
		return ret;

	if (load_8051_fw) {
		ret = load_8051_firmware(dd, &fw_8051);
		if (ret)
			return ret;
	}

	if (load_fabric_fw) {
		ret = load_serdes_firmware(dd, &fw_fabric);
		if (ret)
			return ret;
	}

	/*
	 * TODO: It has been suggested that if the SBUS FW is what enables
	 * PCIE 3.0.  So does this mean that if the SBUS FW is told
	 * not to load, should we reject the load of the PCIe FW?
	 *
	 * TODO: If we're already at Gen3, do we need to do any of this?
	 */
	if (load_sbus_fw) {
		ret = load_sbus_firmware(dd, &fw_sbus);
		if (ret)
			return ret;
	}

	if (load_pcie_fw) {
		ret = load_serdes_firmware(dd, &fw_pcie);
		if (ret)
			return ret;
	}

	dispose_firmware();	/* finished with firmware */

	return 0;
}

//TODO: placeholder
void link_up(struct hfi_devdata *dd)
{
	/*
	 * Reset sequence as described in the DC HAS.  This is the link
	 * bringup, steps 6-10.  Firmware load, steps 1-5, are in the
	 * firmware load function.
	 */

	/*
	 * DC Reset step 6. Load link configuration interactively (optional
	 * - do this if not step 3)
	 */

	/* TODO TBD - need information on values, how to react.
	   If this is out-of-band, we need to be notified that the 8051
	   wants us to do something. */
	/* DC reset step 7. Host request port state from OFFLINE -> POLLING */
	/* TODO: Do we need to do this both IB and STL modes? */

	/* DC reset step 8. STL Mode: */
	/* TODO */

	/* DC reset step 9. STL Mode: */
	/* TODO */

	/* DC reset step 10. STL mode: */
	/* TODO */
}

/*
 * Read the GUID from the hardware, store it in dd.
 */
void read_guid(struct hfi_devdata *dd)
{
	dd->base_guid = read_csr(dd, DC_DC8051_CFG_LOCAL_GUID);
	dd_dev_info(dd, "GUID %llx", (unsigned long long)dd->base_guid);
}
