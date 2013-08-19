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

#include "hfi.h"


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
	const u8 *rsa_n;		/* modulus */
	const u8 *rsa_exponent;
	const u8 *rsa_signature;
	const u8 *rsa_r2;		/* R^2 */
	const u8 *rsa_mu;

	/* payload pieces */
	const u8 *payload;		/* the data */
	u32 payload_len;		/* length in bytes */
};

#define FW_NAME "testcss_dbg.bin"
#define FW_SIGNATURE_DWORDS 64	/* size in DWORDS = 2048 bits */
#define FW_EXPONENT_DWORDS   1	/* size in DWORDS =   32 bits */

#define FW_SIGNATURE_SIZE (FW_SIGNATURE_DWORDS*4)

/* these are the selector values of RSA_VAR_SEL */
#define RSA_SIG 0
#define RSA_N   1


int print_css_header = 1;	/* TODO: hook to verbosity level */
int skip_firmware_load = 1;	/* TODO: temporary */

/*
 * Write data or code to the 8051 code or data RAM.
 */
static int write_8051(struct hfi_devdata *dd, int code, u32 start, const u8 *data, u32 len, struct wstats *wstat)
{
	u64 reg;
	u32 offset;
	unsigned long timeout;
	unsigned long nreads;
	struct wstats local_wstat;
	int err = 0;

	/* init stats */
	if (!wstat)
		wstat = &local_wstat;
	memset(wstat, 0, sizeof(*wstat));
	wstat->min_checks = -1;

	/* we expect 8-byte aligned, length data */
	if (((unsigned long)data % 8) != 0) {
		qib_dev_err(dd, "8501 write: data is not 8-byte aligned\n");
		return -EINVAL;
	}
	if ((len % 8) != 0) {
		qib_dev_err(dd,
			"8501 write: data lengh is not a multiple of 8\n");
		return -EINVAL;
	}

	/* write set-up */
	reg = (code ? DC_DC8051_CFG_RAM_ACCESS_SETUP_RAM_SEL_SMASK : 0ull)
		| DC_DC8051_CFG_RAM_ACCESS_SETUP_AUTO_INCR_ADDR_SMASK;
	write_csr(dd, DC_DC8051_CFG_RAM_ACCESS_SETUP, reg);

	reg = ((start & DC_DC8051_CFG_RAM_ACCESS_CTRL_ADDRESS_MASK)
			<< DC_DC8051_CFG_RAM_ACCESS_CTRL_ADDRESS_SHIFT)
		| DC_DC8051_CFG_RAM_ACCESS_CTRL_WRITE_ENA_SMASK;
	write_csr(dd, DC_DC8051_CFG_RAM_ACCESS_CTRL, reg);

	/* write */
	/* FIXME: need to add a security block settle */
	for (offset = 0; offset < len; offset += 8) {
		if (unlikely(len - offset < 8)) {
			/* handle the non-8-byte aligned case */
			reg = 0;
			memcpy(&reg, &data[offset], len - offset);
		} else {
			reg = *(u64 *)&data[offset];
		}
		write_csr(dd, DC_DC8051_CFG_RAM_ACCESS_WR_DATA, reg);
		/* wait for completion, timeout at 1/10 second */
		timeout = jiffies + (HZ/10 == 0 ? 1 : HZ/10);
		nreads = 0;
		do {
			if (time_after(jiffies, timeout)) {
				qib_dev_err(dd, "8501 write: write fails to complete at address 0x%x, giving up\n",
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
	}

done:
	/* turn off write access, auto increment (also sets to data) */
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

	qib_dev_err(dd, "invalid firmware header field %s: expected 0x%x, actual 0x%x\n", what, expected, actual);
	return 1;
}

/*
 * Request the firmware from the system.  Extract the pieces and fill in
 * fdet.  If succsessful, the caller will need to call dispose_firmware().
 * Returns 0 on success, -ERRNO on error.
 */
static int obtain_firmware(struct hfi_devdata *dd, const char *name,
			struct firmware_details *fdet)
{
	struct css_header *css;
	const u8 *base;
	long prefix_total;
	int ret;

	memset(fdet, 0, sizeof(*fdet));

	ret = request_firmware(&fdet->fw, name, &dd->pcidev->dev);
	if (ret) {
		qib_dev_err(dd, "cannot load firmware \"%s\", err %d\n",
			name, ret);
		return ret;
	}

	/* verify the firmware */
	if (fdet->fw->size < sizeof(struct css_header)) {
		qib_dev_err(dd, "firmware \"%s\" is too small\n", name);
		ret = -EINVAL;
		goto done;
	}
	css = (struct css_header *)fdet->fw->data;
	if (print_css_header) {
		dd_dev_info(dd, "Firmware %s details:\n", name);
		dd_dev_info(dd, "size: %ld bytes\n", fdet->fw->size);
		dd_dev_info(dd, "CSS structure:\n");
		dd_dev_info(dd, "  module_type    0x%x\n", css->module_type);
		dd_dev_info(dd, "  header_len     0x%x\n", css->header_len);
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

	/* verify CSS header fields */
	if (invalid_header(dd, "module_type", css->module_type, CSS_MODULE_TYPE)
			|| invalid_header(dd, "header_len",
					css->header_len, CSS_HEADER_LEN)
			|| invalid_header(dd, "header_version",
					css->header_version, CSS_HEADER_VERSION)
			|| invalid_header(dd, "module_vendor",
					css->module_vendor, CSS_MODULE_VENDOR)
			|| invalid_header(dd, "key_size",
					css->key_size, FW_SIGNATURE_DWORDS)
			|| invalid_header(dd, "modulus_size",
					css->modulus_size, FW_SIGNATURE_DWORDS)
			|| invalid_header(dd, "exponent_size",
					css->exponent_size, FW_EXPONENT_DWORDS)) {
		ret = -EINVAL;
		goto done;
	}

	/*
	 * Firmware file format:
	 *
	 * What				Size (bytes)
	 * ---------------------------	----------------------
	 * CSS header			sizeof(struct css_header)
	 * public key modulues (rsa_n)	FW_SIGNATURE_DWORDS*4
	 * public key exponent		FW_EXPONENT_DWORDS*4
	 * signature			FW_SIGNATURE_DWORDS*4
	 * payload			N
	 */

	/* now that we know more, recheck the file size */
	prefix_total = sizeof(struct css_header)
			+ FW_SIGNATURE_DWORDS*4
			+ FW_EXPONENT_DWORDS *4
			+ FW_SIGNATURE_DWORDS;
	if (prefix_total >= fdet->fw->size) {
		qib_dev_err(dd, "firmware \"%s\", size %ld, must be larger than %ld bytes\n", name, fdet->fw->size, prefix_total);
		ret = -EINVAL;
		goto done;
	}

	/* extract the fields */
	base = fdet->fw->data + sizeof(struct css_header);
	fdet->rsa_n = base;
	base += FW_SIGNATURE_DWORDS * 4;
	fdet->rsa_exponent = base;
	base += FW_EXPONENT_DWORDS * 4;
	fdet->rsa_signature = base;
	base += FW_SIGNATURE_DWORDS * 4;
	fdet->payload = base;
	fdet->payload_len = fdet->fw->size - (base - fdet->fw->data);

	/* TODO: need r2 and mu signatures */

done:
	/* if returning an error, clean up after ourselves */
	if (ret)
		release_firmware(fdet->fw);
	return ret;
}

static void dispose_firmware(struct firmware_details *fdet)
{
	release_firmware(fdet->fw);
}

/*
 * The "what" determines whether we need to stall or not.
 * Returns 0 on success, -ERRNO on failure.
 */
static int write_rsa_data(struct hfi_devdata *dd, int what, const char *name, const u8 *data)
{
	u64 *ptr = (u64 *)data;
	u64 reg;
	unsigned long timeout;
	int i, do_stall;

	/* TODO: stall may not be needed, check back later */
	do_stall = (what == RSA_N);

	/* set the selector, leave everything else at 0 */
	reg = (what & WFR_FW_CTRL_RSA_VAR_SEL_MASK)
				<< WFR_FW_CTRL_RSA_VAR_SEL_SHIFT;
	write_csr(dd, WFR_FW_CTRL, reg);

	/* timeout at 1/10 second */
	timeout = jiffies + (HZ/10 == 0 ? 1 : HZ/10);
	for (ptr = (u64 *)data, i = 0; i < FW_SIGNATURE_SIZE/64; i++, ptr++) {
		write_csr(dd, WFR_RSA_VAR_DATA, *ptr);
		if (do_stall && (i % 8 == 7)) {
			/* wait for the stall to clear */
			/* TODO: add a timeout */
			do {
				if (time_after(jiffies, timeout)) {
					qib_dev_err(dd, "security block stall timeout while writing %s\n", name);
					return -ETIMEDOUT;
				}
				reg = read_csr(dd, WFR_FW_CTRL);
			} while (reg & WFR_FW_CTRL_SHA_STALL_SMASK);
		}
	}

	return 0;
}

/*
 * Load the 8051 firmware.
 * TODO: fabric SerDes, PCIe serDes
 */
int load_firmware(struct hfi_devdata *dd)
{
	u64 reg, firmware;
	struct firmware_details fdet;
	struct wstats wstats;
	unsigned long timeout;
	int ret;

	/* we cannot do anything if we don't have any firmware */
	ret = obtain_firmware(dd, FW_NAME, &fdet);
	/* TODO: TEMPORARY start */
	if (skip_firmware_load) {
		if (!ret) {
			dd_dev_info(dd, "skipping firmware load");
			goto done;
		}
		dd_dev_info(dd, "ignoring missing firmware");
		return 0;
	}
	/* TODO: TEMPORARY end */
	if (ret)
		return ret;

	/* TODO: Do I need to clear the bits in DC_DC8051_ERR_EN? */
	/* TODO: What are the error interrupt hookups from DC to WFR
	   sources?  (Mark Debbage has a sticky note on this in the
	   DC HAS).  ISSUE: Why is it "ERR_EN" but you need to _clear_
	   the bits to enable it?  It seems backward. */
	/* TODO: How can I tell if it is in STL or IB mode? */

	/*
	 * 1. Reset all
	 */
	reg = DC_DC8051_CFG_RST_M8051W_SMASK
		| DC_DC8051_CFG_RST_CRAM_SMASK
		| DC_DC8051_CFG_RST_DRAM_SMASK
		| DC_DC8051_CFG_RST_IRAM_SMASK
		| DC_DC8051_CFG_RST_SFR_SMASK;
	write_csr(dd, DC_DC8051_CFG_RST, reg);

	/*
	 * 2. Load firmware
	 */
	/* release all but the core reset */
	reg = DC_DC8051_CFG_RST_M8051W_SMASK;
	write_csr(dd, DC_DC8051_CFG_RST, reg);

	/* TODO: firmware start of 0? */
	ret = write_8051(dd, 1/*code*/, 0, fdet.payload,
						fdet.payload_len, &wstats);
	if (ret)
		goto done;

	/* TODO: guard with verbosity level */
	dd_dev_info(dd, "8051 firmware download stats:");
	dd_dev_info(dd, "  writes:         %u\n", fdet.payload_len/8);
	dd_dev_info(dd, "  checks:         %ld\n", wstats.nchecks);
	dd_dev_info(dd, "  min check loop: %ld\n", wstats.min_checks);
	dd_dev_info(dd, "  max check loop: %ld\n", wstats.max_checks);

	/*
	 * 3. Load link configuration (optional)
	 */
	/* TODO TBD - where do we get these values? */

	/*
	 * 4. Start 8051
	 */
#ifdef CRYPTO_INACTIVE
	/* clear all reset bits, releasing the 8051 */
	write_csr(dd, DC_DC8051_CFG_RST, 0ull);
#else
	ret = write_rsa_data(dd, RSA_SIG, "signature", fdet.rsa_signature);
	if (ret)
		goto done;
	ret = write_rsa_data(dd, RSA_N, "n (modulus)", fdet.rsa_n);
	if (ret)
		goto done;
	/* TODO: rsa_r2? */
	/* TODO: rsa_mu? */
	/* no need to write rsa_exponent - it is hard-coded */
	/* TODO: ask security block to release the 8051 */
#endif

	/*
	 * 5. Wait for firmware
	 */
	/* timeout at 1/10 second */
	timeout = jiffies + (HZ/10 == 0 ? 1 : HZ/10);
	firmware = 0;
	do {
		if (time_after(jiffies, timeout)) {
			qib_dev_err(dd, "8051 start timeout, current state 0x%llx\n", firmware);
			ret = -ETIMEDOUT;
			goto done;
		}
		reg = read_csr(dd, DC_DC8051_STS_CUR_STATE);
		firmware = (reg >> DC_DC8051_STS_CUR_STATE_FIRMWARE_SHIFT)
				& DC_DC8051_STS_CUR_STATE_FIRMWARE_MASK;
	} while (firmware != 0xa0);	/* ready for HOST request */


	/* FIXME: if nothing is plugged in, then the SerDes are not
	   going to make it to polling.  Handle this case.  We should
	   be able to sucessfully finish loading the driver even with
	   the SerDes offline.  The driver should know if the SerDes
	   are active or not.  The rest of these steps would need to
	   happen out of the normal flow of driver start-up. */

	/*
	 * 6. Load link configuration interactively (optional - do this if
	 * not step 3)
	 */
	/* TODO TBD - need information on values, how to react.
	   If this is out-of-band, we need to be notified that the 8051
	   wants us to do something. */


	/* 7. Host request port state from OFFLINE -> POLLING */
	/* TODO: Do we need to do this both IB and STL modes? */

	/* 8. STL Mode: */
	/* TODO */

	/* 9. STL Mode: */
	/* TODO */

	/* 10. STL mode: */
	/* TODO */

done:
	dispose_firmware(&fdet);
	return ret;
}
