/*
 * Copyright (c) 2006, 2007, 2008, 2009 QLogic Corporation. All rights reserved.
 * Copyright (c) 2003, 2004, 2005, 2006 PathScale, Inc. All rights reserved.
 * Copyright (c) 2015 Intel Corporation.  All rights reserved.
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

#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/vmalloc.h>

#include "hfi.h"
#include "qsfp.h"
#include "twsi.h"

/*
 * QSFP support for hfi driver, using "Two Wire Serial Interface" driver
 * in twsi.c
 */
#define I2C_MAX_RETRY 4

/*
 * Unlocked i2c write.  Must hold dd->qsfp_i2c_mutex.
 */
static int __i2c_write(struct qib_pportdata *ppd, u32 target, int i2c_addr,
		int offset, void *bp, int len)
{
	struct hfi_devdata *dd = ppd->dd;
	int ret, cnt;
	u8 *buff = bp;

	/* Make sure TWSI bus is in sane state. */
	ret = qib_twsi_reset(dd, target);
	if (ret) {
		qib_dev_porterr(dd, ppd->port,
				"I2C interface Reset for write failed\n");
		return -EIO;
	}

	cnt = 0;
	while (cnt < len) {
		int wlen = len - cnt;

		ret = qib_twsi_blk_wr(dd, target, i2c_addr, offset,
							buff + cnt, wlen);
		if (ret) {
			/* qib_twsi_blk_wr() 1 for error, else 0 */
			return -EIO;
		}
		offset += wlen;
		cnt += wlen;
	}

	/* Must wait min 20us between qsfp i2c transactions */
	udelay(20);

	return cnt;
}

int i2c_write(struct qib_pportdata *ppd, u32 target, int i2c_addr, int offset,
		void *bp, int len)
{
	struct hfi_devdata *dd = ppd->dd;
	int ret;

	ret = mutex_lock_interruptible(&dd->qsfp_i2c_mutex);
	if (!ret) {
		ret = __i2c_write(ppd, target, i2c_addr, offset, bp, len);
		mutex_unlock(&dd->qsfp_i2c_mutex);
	}

	return ret;
}

/*
 * Unlocked i2c read.  Must hold dd->qsfp_i2c_mutex.
 */
static int __i2c_read(struct qib_pportdata *ppd, u32 target, int i2c_addr,
			int offset, void *bp, int len)
{
	struct hfi_devdata *dd = ppd->dd;
	int ret, cnt, pass = 0;
	int stuck = 0;
	u8 *buff = bp;

	/* Make sure TWSI bus is in sane state. */
	ret = qib_twsi_reset(dd, target);
	if (ret) {
		qib_dev_porterr(dd, ppd->port,
				"I2C interface Reset for read failed\n");
		ret = -EIO;
		stuck = 1;
		goto exit;
	}

	cnt = 0;
	while (cnt < len) {
		int rlen = len - cnt;

		ret = qib_twsi_blk_rd(dd, target, i2c_addr, offset,
							buff + cnt, rlen);
		/* Some QSFP's fail first try. Retry as experiment */
		if (ret && cnt == 0 && ++pass < I2C_MAX_RETRY)
			continue;
		if (ret) {
			/* qib_twsi_blk_rd() 1 for error, else 0 */
			ret = -EIO;
			goto exit;
		}
		offset += rlen;
		cnt += rlen;
	}

	ret = cnt;

exit:
	if (stuck)
		dd_dev_err(dd, "I2C interface bus stuck non-idle\n");

	if (pass >= I2C_MAX_RETRY && ret)
		qib_dev_porterr(dd, ppd->port,
					"I2C failed even retrying\n");
	else if (pass)
		qib_dev_porterr(dd, ppd->port, "I2C retries: %d\n", pass);

	/* Must wait min 20us between qsfp i2c transactions */
	udelay(20);

	return ret;
}

int i2c_read(struct qib_pportdata *ppd, u32 target, int i2c_addr, int offset,
	     void *bp, int len)
{
	struct hfi_devdata *dd = ppd->dd;
	int ret;

	ret = mutex_lock_interruptible(&dd->qsfp_i2c_mutex);
	if (!ret) {
		ret = __i2c_read(ppd, target, i2c_addr, offset, bp, len);
		mutex_unlock(&dd->qsfp_i2c_mutex);
	}

	return ret;
}

int qsfp_write(struct qib_pportdata *ppd, u32 target, int addr, void *bp,
		int len)
{
	int count = 0;
	int offset;
	int nwrite;
	int ret;
	u8 page;

	ret = mutex_lock_interruptible(&ppd->dd->qsfp_i2c_mutex);
	if (ret)
		return ret;

	while (count < len) {
		/*
		 * Set the qsfp page based on a zero-based addresss
		 * and a page size of QSFP_PAGESIZE bytes.
		 */
		page = (u8)(addr / QSFP_PAGESIZE);
		ret = __i2c_write(ppd, target, QSFP_DEV,
					QSFP_PAGE_SELECT_BYTE_OFFS, &page, 1);
		if (ret != 1) {
			qib_dev_porterr(ppd->dd, ppd->port,
				"can't write QSFP_PAGE_SELECT_BYTE: %d\n", ret);
			ret = -EIO;
			break;
		}

		/* truncate write to end of page if crossing page boundary */
		offset = addr % QSFP_PAGESIZE;
		nwrite = len - count;
		if ((offset + nwrite) > QSFP_PAGESIZE)
			nwrite = QSFP_PAGESIZE - offset;

		ret = __i2c_write(ppd, target, QSFP_DEV, offset, bp + count,
					nwrite);
		if (ret <= 0)	/* stop on error or nothing read */
			break;

		count += ret;
		addr += ret;
	}

	mutex_unlock(&ppd->dd->qsfp_i2c_mutex);

	if (ret < 0)
		return ret;
	return count;
}

int qsfp_read(struct qib_pportdata *ppd, u32 target, int addr, void *bp,
		int len)
{
	int count = 0;
	int offset;
	int nread;
	int ret;
	u8 page;

	ret = mutex_lock_interruptible(&ppd->dd->qsfp_i2c_mutex);
	if (ret)
		return ret;

	while (count < len) {
		/*
		 * Set the qsfp page based on a zero-based address
		 * and a page size of QSFP_PAGESIZE bytes.
		 */
		page = (u8)(addr / QSFP_PAGESIZE);
		ret = __i2c_write(ppd, target, QSFP_DEV,
					QSFP_PAGE_SELECT_BYTE_OFFS, &page, 1);
		if (ret != 1) {
			qib_dev_porterr(ppd->dd, ppd->port,
				"can't write QSFP_PAGE_SELECT_BYTE: %d\n", ret);
			ret = -EIO;
			break;
		}

		/* truncate read to end of page if crossing page boundary */
		offset = addr % QSFP_PAGESIZE;
		nread = len - count;
		if ((offset + nread) > QSFP_PAGESIZE)
			nread = QSFP_PAGESIZE - offset;

		ret = __i2c_read(ppd, target, QSFP_DEV, offset, bp + count,
					nread);
		if (ret <= 0)	/* stop on error or nothing read */
			break;

		count += ret;
		addr += ret;
	}

	mutex_unlock(&ppd->dd->qsfp_i2c_mutex);

	if (ret < 0)
		return ret;
	return count;
}

/*
 * For validation, we want to check the checksums, even of the
 * fields we do not otherwise use. This function reads the bytes from
 * <first> to <next-1> and returns the 8lsbs of the sum, or <0 for errors
 */
static int qsfp_cks(struct qib_pportdata *ppd, u32 target, int first, int next)
{
	int ret;
	u16 cks;
	u8 bval;

	cks = 0;
	while (first < next) {
		ret = qsfp_read(ppd, target, first, &bval, 1);
		if (ret < 0)
			goto bail;
		cks += bval;
		++first;
	}
	ret = cks & 0xFF;
bail:
	return ret;

}

int qib_refresh_qsfp_cache(struct qib_pportdata *ppd, struct qib_qsfp_cache *cp)
{
	u32 target = ppd->dd->hfi_id;
	int ret;
	int idx;
	u16 cks;
	u8 byte0;

	/* ensure sane contents on invalid reads, for cable swaps */
	memset(cp, 0, sizeof(*cp));

	if (!qsfp_mod_present(ppd)) {
		ret = -ENODEV;
		goto bail;
	}

	ret = qsfp_read(ppd, target, 0, &byte0, 1);
	if (ret < 0)
		goto bail;
	if ((byte0 & 0xFE) != 0x0C)
		qib_dev_porterr(ppd->dd, ppd->port,
				"QSFP byte0 is 0x%02X, S/B 0x0C/D\n", byte0);

	ret = qsfp_read(ppd, target, QSFP_MOD_ID_OFFS, &cp->id, 1);
	if (ret < 0)
		goto bail;
	if ((cp->id & 0xFE) != 0x0C)
		qib_dev_porterr(ppd->dd, ppd->port,
				"QSFP ID byte is 0x%02X, S/B 0x0C/D\n", cp->id);
	cks = cp->id;

	ret = qsfp_read(ppd, target, QSFP_MOD_PWR_OFFS, &cp->pwr, 1);
	if (ret < 0)
		goto bail;
	cks += cp->pwr;

	ret = qsfp_cks(ppd, target, QSFP_MOD_PWR_OFFS + 1, QSFP_MOD_LEN_OFFS);
	if (ret < 0)
		goto bail;
	cks += ret;

	ret = qsfp_read(ppd, target, QSFP_MOD_LEN_OFFS, &cp->len, 1);
	if (ret < 0)
		goto bail;
	cks += cp->len;

	ret = qsfp_read(ppd, target, QSFP_MOD_TECH_OFFS, &cp->tech, 1);
	if (ret < 0)
		goto bail;
	cks += cp->tech;

	ret = qsfp_read(ppd, target, QSFP_VEND_OFFS, &cp->vendor,
			QSFP_VEND_LEN);
	if (ret < 0)
		goto bail;
	for (idx = 0; idx < QSFP_VEND_LEN; ++idx)
		cks += cp->vendor[idx];

	ret = qsfp_read(ppd, target, QSFP_IBXCV_OFFS, &cp->xt_xcv, 1);
	if (ret < 0)
		goto bail;
	cks += cp->xt_xcv;

	ret = qsfp_read(ppd, target, QSFP_VOUI_OFFS, &cp->oui, QSFP_VOUI_LEN);
	if (ret < 0)
		goto bail;
	for (idx = 0; idx < QSFP_VOUI_LEN; ++idx)
		cks += cp->oui[idx];

	ret = qsfp_read(ppd, target, QSFP_PN_OFFS, &cp->partnum, QSFP_PN_LEN);
	if (ret < 0)
		goto bail;
	for (idx = 0; idx < QSFP_PN_LEN; ++idx)
		cks += cp->partnum[idx];

	ret = qsfp_read(ppd, target, QSFP_REV_OFFS, &cp->rev, QSFP_REV_LEN);
	if (ret < 0)
		goto bail;
	for (idx = 0; idx < QSFP_REV_LEN; ++idx)
		cks += cp->rev[idx];

	ret = qsfp_read(ppd, target, QSFP_ATTEN_OFFS, &cp->atten,
			QSFP_ATTEN_LEN);
	if (ret < 0)
		goto bail;
	for (idx = 0; idx < QSFP_ATTEN_LEN; ++idx)
		cks += cp->atten[idx];

	ret = qsfp_cks(ppd, target, QSFP_ATTEN_OFFS + QSFP_ATTEN_LEN,
			QSFP_CC_OFFS);
	if (ret < 0)
		goto bail;
	cks += ret;

	cks &= 0xFF;
	ret = qsfp_read(ppd, target, QSFP_CC_OFFS, &cp->cks1, 1);
	if (ret < 0)
		goto bail;
	if (cks != cp->cks1)
		qib_dev_porterr(ppd->dd, ppd->port,
				"QSFP cks1 is %02X, computed %02X\n", cp->cks1,
				cks);

	/* Second checksum covers 192 to (serial, date, lot) */
	ret = qsfp_cks(ppd, target, QSFP_CC_OFFS + 1, QSFP_SN_OFFS);
	if (ret < 0)
		goto bail;
	cks = ret;

	ret = qsfp_read(ppd, target, QSFP_SN_OFFS, &cp->serial, QSFP_SN_LEN);
	if (ret < 0)
		goto bail;
	for (idx = 0; idx < QSFP_SN_LEN; ++idx)
		cks += cp->serial[idx];

	ret = qsfp_read(ppd, target, QSFP_DATE_OFFS, &cp->date, QSFP_DATE_LEN);
	if (ret < 0)
		goto bail;
	for (idx = 0; idx < QSFP_DATE_LEN; ++idx)
		cks += cp->date[idx];

	ret = qsfp_read(ppd, target, QSFP_LOT_OFFS, &cp->lot, QSFP_LOT_LEN);
	if (ret < 0)
		goto bail;
	for (idx = 0; idx < QSFP_LOT_LEN; ++idx)
		cks += cp->lot[idx];

	ret = qsfp_cks(ppd, target, QSFP_LOT_OFFS + QSFP_LOT_LEN,
			QSFP_CC_EXT_OFFS);
	if (ret < 0)
		goto bail;
	cks += ret;

	ret = qsfp_read(ppd, target, QSFP_CC_EXT_OFFS, &cp->cks2, 1);
	if (ret < 0)
		goto bail;
	cks &= 0xFF;
	if (cks != cp->cks2)
		qib_dev_porterr(ppd->dd, ppd->port,
				"QSFP cks2 is %02X, computed %02X\n", cp->cks2,
				cks);
	return 0;

bail:
	cp->id = 0;
	return ret;
}

const char * const qib_qsfp_devtech[16] = {
	"850nm VCSEL", "1310nm VCSEL", "1550nm VCSEL", "1310nm FP",
	"1310nm DFB", "1550nm DFB", "1310nm EML", "1550nm EML",
	"Cu Misc", "1490nm DFB", "Cu NoEq", "Cu Eq",
	"Undef", "Cu Active BothEq", "Cu FarEq", "Cu NearEq"
};

#define QSFP_DUMP_CHUNK 16 /* Holds longest string */
#define QSFP_DEFAULT_HDR_CNT 224

static const char *pwr_codes = "1.5W2.0W2.5W3.5W";

int qsfp_mod_present(struct qib_pportdata *ppd)
{
	if (HFI_CAP_IS_KSET(QSFP_ENABLED)) {
		struct hfi_devdata *dd = ppd->dd;
		u64 reg;

		reg = read_csr(dd,
			dd->hfi_id ? WFR_ASIC_QSFP2_IN : WFR_ASIC_QSFP1_IN);
		return !(reg & QSFP_HFI0_MODPRST_N);
	}
	/* always return cable present */
	return 1;
}

/*
 * Initialize structures that control access to QSFP. Called once per port.
 */
void qib_qsfp_init(struct qib_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;
	int ret;

	/*
	 * A0 leaves the out lines floating on power on, then on an FLR
	 * enforces a 0 on all out pins.  The driver does not touch
	 * ASIC_QSFPn_OUT otherwise.  This leaves RESET_N low and anything
	 * plugged constantly in reset, if it pays attention to RESET_N.
	 * The prime example of this is SiPh.  For now, set all pins high.
	 * I2CCLK and I2CDAT will change per direction, and INT_N and
	 * MODPRS_N are input only and their value is ignored.
	 *
	 * We write both outputs because we allow access to both i2c chains
	 * from each HFI.  There are possible interference issues if this
	 * code is run while the other HFI is doing an access.  Because of
	 * the mutex, this can only happen if the other HFI is running on
	 * another operating system.
	 */
	if (is_a0(dd)) {
		ret = mutex_lock_interruptible(&dd->qsfp_i2c_mutex);
		if (ret) {
			/* complain, but otherwise don't do anything */
			dd_dev_err(dd, "Cannot set QSFP pins high\n");
		} else {
			write_csr(dd, WFR_ASIC_QSFP1_OUT, 0x1f);
			write_csr(dd, WFR_ASIC_QSFP2_OUT, 0x1f);
			mutex_unlock(&dd->qsfp_i2c_mutex);
		}
	}
}

int qib_qsfp_dump(struct qib_pportdata *ppd, char *buf, int len)
{
	struct qib_qsfp_cache cd;
	u8 bin_buff[QSFP_DUMP_CHUNK];
	char lenstr[6];
	int sofar, ret;
	int bidx = 0;

	sofar = 0;
	ret = qib_refresh_qsfp_cache(ppd, &cd);
	if (ret < 0)
		goto bail;

	lenstr[0] = ' ';
	lenstr[1] = '\0';
	if (QSFP_IS_CU(cd.tech))
		sprintf(lenstr, "%dM ", cd.len);

	sofar += scnprintf(buf + sofar, len - sofar, "PWR:%.3sW\n", pwr_codes +
			   (QSFP_PWR(cd.pwr) * 4));

	sofar += scnprintf(buf + sofar, len - sofar, "TECH:%s%s\n", lenstr,
			   qib_qsfp_devtech[cd.tech >> 4]);

	sofar += scnprintf(buf + sofar, len - sofar, "Vendor:%.*s\n",
			   QSFP_VEND_LEN, cd.vendor);

	sofar += scnprintf(buf + sofar, len - sofar, "OUI:%06X\n",
			   QSFP_OUI(cd.oui));

	sofar += scnprintf(buf + sofar, len - sofar, "Part#:%.*s\n",
			   QSFP_PN_LEN, cd.partnum);
	sofar += scnprintf(buf + sofar, len - sofar, "Rev:%.*s\n",
			   QSFP_REV_LEN, cd.rev);
	if (QSFP_IS_CU(cd.tech))
		sofar += scnprintf(buf + sofar, len - sofar, "Atten:%d, %d\n",
				   QSFP_ATTEN_SDR(cd.atten),
				   QSFP_ATTEN_DDR(cd.atten));
	sofar += scnprintf(buf + sofar, len - sofar, "Serial:%.*s\n",
			   QSFP_SN_LEN, cd.serial);
	sofar += scnprintf(buf + sofar, len - sofar, "Date:%.*s\n",
			   QSFP_DATE_LEN, cd.date);
	sofar += scnprintf(buf + sofar, len - sofar, "Lot:%.*s\n",
			   QSFP_LOT_LEN, cd.date);

	while (bidx < QSFP_DEFAULT_HDR_CNT) {
		int iidx;

		ret = qsfp_read(ppd, ppd->dd->hfi_id, bidx, bin_buff,
				QSFP_DUMP_CHUNK);
		if (ret < 0)
			goto bail;
		for (iidx = 0; iidx < ret; ++iidx) {
			sofar += scnprintf(buf + sofar, len-sofar, " %02X",
				bin_buff[iidx]);
		}
		sofar += scnprintf(buf + sofar, len - sofar, "\n");
		bidx += QSFP_DUMP_CHUNK;
	}
	ret = sofar;
bail:
	return ret;
}
