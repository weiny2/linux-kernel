/*
 * Copyright(c) 2017 Intel Corporation.
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
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

#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/vmalloc.h>
#include "chip/fxr_top_defs.h"
#include "chip/fxr_gbl_defs.h"
#include "hfi2.h"
#include "link.h"

static const u8 test_qsfp = 1;

static int hfi_handle_qsfp_error_conditions(struct hfi_pportdata *ppd,
					    u8 *qsfp_interrupt_status)
{
	struct hfi_devdata *dd = ppd->dd;

	if ((qsfp_interrupt_status[0] & QSFP_HIGH_TEMP_ALARM) ||
	    (qsfp_interrupt_status[0] & QSFP_HIGH_TEMP_WARNING))
		dd_dev_info(dd, "%s: QSFP cable temperature too high\n",
			    __func__);

	if ((qsfp_interrupt_status[0] & QSFP_LOW_TEMP_ALARM) ||
	    (qsfp_interrupt_status[0] & QSFP_LOW_TEMP_WARNING))
		dd_dev_info(dd, "%s: QSFP cable temperature too low\n",
			    __func__);

	/*
	 * The remaining alarms/warnings don't matter if the link is down.
	 */
	if (ppd->host_link_state & HLS_DOWN)
		return 0;

	if ((qsfp_interrupt_status[1] & QSFP_HIGH_VCC_ALARM) ||
	    (qsfp_interrupt_status[1] & QSFP_HIGH_VCC_WARNING))
		dd_dev_info(dd, "%s: QSFP supply voltage too high\n",
			    __func__);

	if ((qsfp_interrupt_status[1] & QSFP_LOW_VCC_ALARM) ||
	    (qsfp_interrupt_status[1] & QSFP_LOW_VCC_WARNING))
		dd_dev_info(dd, "%s: QSFP supply voltage too low\n",
			    __func__);

	/* Byte 2 is vendor specific */

	if ((qsfp_interrupt_status[3] & QSFP_HIGH_POWER_ALARM) ||
	    (qsfp_interrupt_status[3] & QSFP_HIGH_POWER_WARNING))
		dd_dev_info(dd, "%s: Cable RX channel 1/2 power too high\n",
			    __func__);

	if ((qsfp_interrupt_status[3] & QSFP_LOW_POWER_ALARM) ||
	    (qsfp_interrupt_status[3] & QSFP_LOW_POWER_WARNING))
		dd_dev_info(dd, "%s: Cable RX channel 1/2 power too low\n",
			    __func__);

	if ((qsfp_interrupt_status[4] & QSFP_HIGH_POWER_ALARM) ||
	    (qsfp_interrupt_status[4] & QSFP_HIGH_POWER_WARNING))
		dd_dev_info(dd, "%s: Cable RX channel 3/4 power too high\n",
			    __func__);

	if ((qsfp_interrupt_status[4] & QSFP_LOW_POWER_ALARM) ||
	    (qsfp_interrupt_status[4] & QSFP_LOW_POWER_WARNING))
		dd_dev_info(dd, "%s: Cable RX channel 3/4 power too low\n",
			    __func__);

	if ((qsfp_interrupt_status[5] & QSFP_HIGH_BIAS_ALARM) ||
	    (qsfp_interrupt_status[5] & QSFP_HIGH_BIAS_WARNING))
		dd_dev_info(dd, "%s: Cable TX channel 1/2 bias too high\n",
			    __func__);

	if ((qsfp_interrupt_status[5] & QSFP_LOW_BIAS_ALARM) ||
	    (qsfp_interrupt_status[5] & QSFP_LOW_BIAS_WARNING))
		dd_dev_info(dd, "%s: Cable TX channel 1/2 bias too low\n",
			    __func__);

	if ((qsfp_interrupt_status[6] & QSFP_HIGH_BIAS_ALARM) ||
	    (qsfp_interrupt_status[6] & QSFP_HIGH_BIAS_WARNING))
		dd_dev_info(dd, "%s: Cable TX channel 3/4 bias too high\n",
			    __func__);

	if ((qsfp_interrupt_status[6] & QSFP_LOW_BIAS_ALARM) ||
	    (qsfp_interrupt_status[6] & QSFP_LOW_BIAS_WARNING))
		dd_dev_info(dd, "%s: Cable TX channel 3/4 bias too low\n",
			    __func__);

	if ((qsfp_interrupt_status[7] & QSFP_HIGH_POWER_ALARM) ||
	    (qsfp_interrupt_status[7] & QSFP_HIGH_POWER_WARNING))
		dd_dev_info(dd, "%s: Cable TX channel 1/2 power too high\n",
			    __func__);

	if ((qsfp_interrupt_status[7] & QSFP_LOW_POWER_ALARM) ||
	    (qsfp_interrupt_status[7] & QSFP_LOW_POWER_WARNING))
		dd_dev_info(dd, "%s: Cable TX channel 1/2 power too low\n",
			    __func__);

	if ((qsfp_interrupt_status[8] & QSFP_HIGH_POWER_ALARM) ||
	    (qsfp_interrupt_status[8] & QSFP_HIGH_POWER_WARNING))
		dd_dev_info(dd, "%s: Cable TX channel 3/4 power too high\n",
			    __func__);

	if ((qsfp_interrupt_status[8] & QSFP_LOW_POWER_ALARM) ||
	    (qsfp_interrupt_status[8] & QSFP_LOW_POWER_WARNING))
		dd_dev_info(dd, "%s: Cable TX channel 3/4 power too low\n",
			    __func__);

	/*
	 * Bytes 9-10 and 11-12 are reserved
	 * Bytes 13-15 are vendor specific
	 */

	return 0;
}

static void hfi_setsda(void *data, int state)
{
	struct hfi_i2c_bus *bus = (struct hfi_i2c_bus *)data;
	struct hfi_pportdata *ppd = bus->ppd;
	u64 reg;

	reg = read_csr(ppd->dd, FXR_ASIC_QSFP_OE);

	/*
	 * The OE bit value is inverted and connected to the pin.  When
	 * OE is 0 the pin is left to be pulled up, when the OE is 1
	 * the pin is driven low.  This matches the "open drain" or "open
	 * collector" convention.
	 */
	if (state)
		reg &= ~QSFP_HFI_I2CDAT;
	else
		reg |= QSFP_HFI_I2CDAT;
	write_csr(ppd->dd, FXR_ASIC_QSFP_OE, reg);

	/* do a read to force the write into the chip */
	(void)read_csr(ppd->dd, FXR_ASIC_QSFP_OE);
}

static void hfi_setscl(void *data, int state)
{
	struct hfi_i2c_bus *bus = (struct hfi_i2c_bus *)data;
	struct hfi_pportdata *ppd = bus->ppd;
	u64 reg;

	reg = read_csr(ppd->dd, FXR_ASIC_QSFP_OE);

	/*
	 * The OE bit value is inverted and connected to the pin.  When
	 * OE is 0 the pin is left to be pulled up, when the OE is 1
	 * the pin is driven low.  This matches the "open drain" or "open
	 * collector" convention.
	 */
	if (state)
		reg &= ~QSFP_HFI_I2CCLK;
	else
		reg |= QSFP_HFI_I2CCLK;
	write_csr(ppd->dd, FXR_ASIC_QSFP_OE, reg);

	/* do a read to force the write into the chip */
	(void)read_csr(ppd->dd, FXR_ASIC_QSFP_OE);
}

static int hfi_getsda(void *data)
{
	struct hfi_i2c_bus *bus = (struct hfi_i2c_bus *)data;
	u64 reg;

	hfi_setsda(data, 1);	/* clear OE so we do not pull line down */
	udelay(2);		/* 1us pull up + 250ns hold */

	reg = read_csr(bus->ppd->dd, FXR_ASIC_QSFP_IN);
	return !!(reg & QSFP_HFI_I2CDAT);
}

static int hfi_getscl(void *data)
{
	struct hfi_i2c_bus *bus = (struct hfi_i2c_bus *)data;
	u64 reg;

	hfi_setscl(data, 1);	/* clear OE so we do not pull line down */
	udelay(2);		/* 1us pull up + 250ns hold */

	reg = read_csr(bus->ppd->dd, FXR_ASIC_QSFP_IN);
	return !!(reg & QSFP_HFI_I2CCLK);
}

/*
 * Allocate and initialize the given i2c bus number.
 * Returns ERR_PTR(error) on failure.
 */
struct hfi_i2c_bus *hfi_init_i2c_bus(struct hfi_pportdata *ppd, int num)
{
	struct hfi_i2c_bus *bus;
	int ret;

	bus = kzalloc(sizeof(*bus), GFP_KERNEL);
	if (ZERO_OR_NULL_PTR(bus))
		return ERR_PTR(-ENOMEM);

	bus->ppd = ppd;
	bus->num = num;	/* our bus number */

	bus->algo.setsda = hfi_setsda;
	bus->algo.setscl = hfi_setscl;
	bus->algo.getsda = hfi_getsda;
	bus->algo.getscl = hfi_getscl;
	bus->algo.udelay = 5;
	bus->algo.timeout = usecs_to_jiffies(100000);
	bus->algo.data = bus;

	bus->adapter.owner = THIS_MODULE;
	bus->adapter.algo_data = &bus->algo;
	bus->adapter.dev.parent = &ppd->dd->pdev->dev;
	snprintf(bus->adapter.name, sizeof(bus->adapter.name),
		 "%s_i2c%d", hfi_class_name(),  num);

	ret = i2c_bit_add_bus(&bus->adapter);
	if (ret) {
		ppd_dev_info(ppd, "%s: unable to add i2c bus %d, err %d\n",
			     __func__, num, ret);
		kfree(bus);
		return ERR_PTR(ret);
	}

	return bus;
}

void hfi_clean_i2c_bus(struct hfi_i2c_bus *bus)
{
	if (bus) {
		i2c_del_adapter(&bus->adapter);
		kfree(bus);
	}
}

static int hfi_i2c_bus_write(struct hfi_devdata *dd, struct hfi_i2c_bus *i2c,
			     u8 slave_addr, int offset, int offset_size,
			     u8 *data, u16 len)
{
	int ret;
	int num_msgs;
	u8 offset_bytes[2];
	struct i2c_msg msgs[2];

	switch (offset_size) {
	case 0:
		num_msgs = 1;
		msgs[0].addr = slave_addr;
		msgs[0].flags = 0;
		msgs[0].len = len;
		msgs[0].buf = data;
		break;
	case 2:
		offset_bytes[1] = (offset >> 8) & 0xff;
		/* fall through */
	case 1:
		num_msgs = 2;
		offset_bytes[0] = offset & 0xff;

		msgs[0].addr = slave_addr;
		msgs[0].flags = 0;
		msgs[0].len = offset_size;
		msgs[0].buf = offset_bytes;

		msgs[1].addr = slave_addr;
		msgs[1].flags = I2C_M_NOSTART,
		msgs[1].len = len;
		msgs[1].buf = data;
		break;
	default:
		return -EINVAL;
	}

	ret = i2c_transfer(&i2c->adapter, msgs, num_msgs);
	if (ret != num_msgs) {
		dd_dev_err(
			dd,
			"%s: i2c slv 0x%x, offset 0x%x, len 0x%x; failed, ret %d\n",
			__func__, slave_addr, offset, len, ret);
		return ret < 0 ? ret : -EIO;
	}
	return 0;
}

static int hfi_i2c_bus_read(struct hfi_devdata *dd, struct hfi_i2c_bus *bus,
			    u8 slave_addr, int offset, int offset_size,
			    u8 *data, u16 len)
{
	int ret;
	int num_msgs;
	u8 offset_bytes[2];
	struct i2c_msg msgs[2];

	switch (offset_size) {
	case 0:
		num_msgs = 1;
		msgs[0].addr = slave_addr;
		msgs[0].flags = I2C_M_RD;
		msgs[0].len = len;
		msgs[0].buf = data;
		break;
	case 2:
		offset_bytes[1] = (offset >> 8) & 0xff;
		/* fall through */
	case 1:
		num_msgs = 2;
		offset_bytes[0] = offset & 0xff;

		msgs[0].addr = slave_addr;
		msgs[0].flags = 0;
		msgs[0].len = offset_size;
		msgs[0].buf = offset_bytes;

		msgs[1].addr = slave_addr;
		msgs[1].flags = I2C_M_RD,
		msgs[1].len = len;
		msgs[1].buf = data;
		break;
	default:
		return -EINVAL;
	}

	if (test_qsfp) {
		/*
		 * FXRTODO: Remove this section once i2c and qsfp registers
		 * are implemented in simics or hw is available
		 *
		 * Return a prefilled / zeroed buffer instead of calling the
		 * i2c_transfer() function
		 */
		u8 cable_info[256];
		int i = 0;

		for (i = 0; i < 256; i++)
			cable_info[i] = i;

		dd_dev_dbg(dd, "address offset: 0x%x, length: 0x%x\n",
			   offset, len);

		memcpy(data, &cable_info[offset], len);

		return 0;
	}

	ret = i2c_transfer(&bus->adapter, msgs, num_msgs);
	if (ret != num_msgs) {
		dd_dev_err(dd, "%s: i2c slv 0x%x, offset 0x%x, len 0x%x; failed, ret %d\n",
			   __func__, slave_addr, offset, len, ret);
		return ret < 0 ? ret : -EIO;
	}
	return 0;
}

/*
 * Raw i2c write.  No set-up or lock checking.
 *
 * Return 0 on success, -errno on error.
 */
static int hfi_i2c_write(struct hfi_pportdata *ppd, u32 target, int i2c_addr,
			 int offset, void *bp, int len)
{
	struct hfi_devdata *dd = ppd->dd;
	struct hfi_i2c_bus *bus;
	u8 slave_addr;
	int offset_size;

	if (test_qsfp) {
		/*
		 * FXRTODO: Rempove this section once i2c and qsfp registers are
		 * implemented on simics or hw is available
		 */
		return 0;
	}

	bus = ppd->i2c_bus;
	slave_addr = (i2c_addr & 0xff) >> 1; /* convert to 7-bit addr */
	offset_size = (i2c_addr >> 8) & 0x3;
	return hfi_i2c_bus_write(dd, bus, slave_addr, offset,
			offset_size, bp, len);
}

/*
 * Raw i2c read.  No set-up or lock checking.
 *
 * Return 0 on success, -errno on error.
 */
static int hfi_i2c_read(struct hfi_pportdata *ppd, u32 target, int i2c_addr,
			int offset, void *bp, int len)
{
	struct hfi_devdata *dd = ppd->dd;
	struct hfi_i2c_bus *bus;
	u8 slave_addr;
	int offset_size;

	bus = ppd->i2c_bus;
	slave_addr = (i2c_addr & 0xff) >> 1; /* convert to 7-bit addr */
	offset_size = (i2c_addr >> 8) & 0x3;
	return hfi_i2c_bus_read(dd, bus, slave_addr, offset,
			offset_size, bp, len);
}

/*
 * Write page n, offset m of QSFP memory as defined by SFF 8636
 * by writing @addr = ((256 * n) + m)
 *
 * Return number of bytes written or -errno.
 */
int hfi_qsfp_write(struct hfi_pportdata *ppd, u32 target, int addr,
		   void *bp, int len)
{
	int count = 0;
	int offset;
	int nwrite;
	int ret = 0;
	u8 page;

	while (count < len) {
		/*
		 * Set the qsfp page based on a zero-based address
		 * and a page size of QSFP_PAGESIZE bytes.
		 */
		page = (u8)(addr / QSFP_PAGESIZE);

		ret = hfi_i2c_write(ppd, target, QSFP_DEV | QSFP_OFFSET_SIZE,
				    QSFP_PAGE_SELECT_BYTE_OFFS, &page, 1);
		/* QSFPs require a 5-10msec delay after write operations */
		mdelay(5);
		if (ret) {
			ppd_dev_err(ppd, "%s: %d QSFP %d Invalid write %d\n",
				    __func__, __LINE__, target, ret);
			break;
		}

		offset = addr % QSFP_PAGESIZE;
		nwrite = len - count;
		/* truncate write to boundary if crossing boundary */
		if (((addr % QSFP_RW_BOUNDARY) + nwrite) > QSFP_RW_BOUNDARY)
			nwrite = QSFP_RW_BOUNDARY - (addr % QSFP_RW_BOUNDARY);

		ret = hfi_i2c_write(ppd, target, QSFP_DEV | QSFP_OFFSET_SIZE,
				    offset, bp + count, nwrite);
		/* QSFPs require a 5-10msec delay after write operations */
		mdelay(5);
		if (ret)	/* stop on error */
			break;

		count += nwrite;
		addr += nwrite;
	}

	if (ret < 0)
		return ret;
	return count;
}

/*
 * Access page n, offset m of QSFP memory as defined by SFF 8636
 * by reading @addr = ((256 * n) + m)
 *
 * Return the number of bytes read or -errno.
 */
static int hfi_qsfp_read(struct hfi_pportdata *ppd, u32 target, int addr,
			 void *bp, int len)
{
	int count = 0;
	int offset;
	int nread;
	int ret = 0;
	u8 page;

	while (count < len) {
		/*
		 * Set the qsfp page based on a zero-based address
		 * and a page size of QSFP_PAGESIZE bytes.
		 */
		page = (u8)(addr / QSFP_PAGESIZE);
		ret = hfi_i2c_write(ppd, target, QSFP_DEV | QSFP_OFFSET_SIZE,
				    QSFP_PAGE_SELECT_BYTE_OFFS, &page, 1);
		/* QSFPs require a 5-10msec delay after write operations */
		mdelay(5);
		if (ret) {
			ppd_dev_err(ppd, "%s: %d QSFP %d Invalid write %d\n",
				    __func__, __LINE__, target, ret);
			break;
		}

		offset = addr % QSFP_PAGESIZE;
		nread = len - count;
		/* truncate read to boundary if crossing boundary */
		if (((addr % QSFP_RW_BOUNDARY) + nread) > QSFP_RW_BOUNDARY)
			nread = QSFP_RW_BOUNDARY - (addr % QSFP_RW_BOUNDARY);

		ret = hfi_i2c_read(ppd, target, QSFP_DEV | QSFP_OFFSET_SIZE,
				   offset, bp + count, nread);
		if (ret)	/* stop on error */
			break;

		count += nread;
		addr += nread;
	}

	if (ret < 0)
		return ret;
	return count;
}

/*
 * This function caches the QSFP memory range in 128 byte chunks.
 * As an example, the next byte after address 255 is byte 128 from
 * upper page 01H (if existing) rather than byte 0 from lower page 00H.
 * Access page n, offset m of QSFP memory as defined by SFF 8636
 * in the cache by reading byte ((128 * n) + m)
 * The calls to qsfp_{read,write} in this function correctly handle the
 * address map difference between this mapping and the mapping implemented
 * by those functions
 */
int hfi_refresh_qsfp_cache(struct hfi_pportdata *ppd,
			   struct qsfp_data *cp)
{
	u32 target = ppd->dd->hfi_id;
	int ret;
	unsigned long flags;
	u8 *cache = &cp->cache[0];

	/* ensure sane contents on invalid reads, for cable swaps */
	memset(cache, 0, (QSFP_MAX_NUM_PAGES * 128));
	spin_lock_irqsave(&ppd->qsfp_info.qsfp_lock, flags);
	ppd->qsfp_info.cache_valid = 0;
	spin_unlock_irqrestore(&ppd->qsfp_info.qsfp_lock, flags);

	if (!hfi_qsfp_mod_present(ppd)) {
		ret = -ENODEV;
		goto bail;
	}

	ret = hfi_qsfp_read(ppd, target, 0, cache, QSFP_PAGESIZE);
	if (ret != QSFP_PAGESIZE) {
		dd_dev_info(ppd->dd,
			    "%s: Page 0 read failed, expected %d, got %d\n",
			    __func__, QSFP_PAGESIZE, ret);
		goto bail;
	}

	/* Is paging enabled? */
	if (!(cache[2] & 4)) {
		/* Paging enabled, page 03 required */
		if ((cache[195] & 0xC0) == 0xC0) {
			/* all */
			ret = hfi_qsfp_read(ppd, target, 384,
					    cache + 256, 128);
			if (ret <= 0 || ret != 128) {
				dd_dev_info(ppd->dd, "%s failed\n", __func__);
				goto bail;
			}
			ret = hfi_qsfp_read(ppd, target, 640,
					    cache + 384, 128);
			if (ret <= 0 || ret != 128) {
				dd_dev_info(ppd->dd, "%s failed\n", __func__);
				goto bail;
			}
			ret = hfi_qsfp_read(ppd, target, 896,
					    cache + 512, 128);
			if (ret <= 0 || ret != 128) {
				dd_dev_info(ppd->dd, "%s failed\n", __func__);
				goto bail;
			}
		} else if ((cache[195] & 0x80) == 0x80) {
			/* only page 2 and 3 */
			ret = hfi_qsfp_read(ppd, target, 640,
					    cache + 384, 128);
			if (ret <= 0 || ret != 128) {
				dd_dev_info(ppd->dd, "%s failed\n", __func__);
				goto bail;
			}
			ret = hfi_qsfp_read(ppd, target, 896,
					    cache + 512, 128);
			if (ret <= 0 || ret != 128) {
				dd_dev_info(ppd->dd, "%s failed\n", __func__);
				goto bail;
			}
		} else if ((cache[195] & 0x40) == 0x40) {
			/* only page 1 and 3 */
			ret = hfi_qsfp_read(ppd, target, 384,
					    cache + 256, 128);
			if (ret <= 0 || ret != 128) {
				dd_dev_info(ppd->dd, "%s failed\n", __func__);
				goto bail;
			}
			ret = hfi_qsfp_read(ppd, target, 896,
					    cache + 512, 128);
			if (ret <= 0 || ret != 128) {
				dd_dev_info(ppd->dd, "%s failed\n", __func__);
				goto bail;
			}
		} else {
			/* only page 3 */
			ret = hfi_qsfp_read(ppd, target, 896,
					    cache + 512, 128);
			if (ret <= 0 || ret != 128) {
				dd_dev_info(ppd->dd, "%s failed\n", __func__);
				goto bail;
			}
		}
	}

	spin_lock_irqsave(&ppd->qsfp_info.qsfp_lock, flags);
	ppd->qsfp_info.cache_valid = 1;
	ppd->qsfp_info.cache_refresh_required = 0;
	spin_unlock_irqrestore(&ppd->qsfp_info.qsfp_lock, flags);

	return 0;

bail:
	memset(cache, 0, (QSFP_MAX_NUM_PAGES * 128));
	return ret;
}

const char * const hfi_qsfp_devtech[16] = {
	"850nm VCSEL", "1310nm VCSEL", "1550nm VCSEL", "1310nm FP",
	"1310nm DFB", "1550nm DFB", "1310nm EML", "1550nm EML",
	"Cu Misc", "1490nm DFB", "Cu NoEq", "Cu Eq",
	"Undef", "Cu Active BothEq", "Cu FarEq", "Cu NearEq"
};

/*
 * Takes power class byte [Page 00 Byte 129] in SFF 8636
 * Returns power class as integer (1 through 7, per SFF 8636 rev 2.4)
 */
int hfi_get_qsfp_power_class(u8 power_byte)
{
	if (QSFP_HIGH_PWR(power_byte) == QSFP_HIGH_PWR_UNUSED)
		/* power classes count from 1, their bit encodings from 0 */
		return (QSFP_PWR(power_byte) + 1);
	/*
	 * 00 in the high power classes stands for unused, bringing
	 * balance to the off-by-1 offset above, we add 4 here to
	 * account for the difference between the low and high power
	 * groups
	 */
	return (QSFP_HIGH_PWR(power_byte) + 4);
}

int hfi_qsfp_mod_present(struct hfi_pportdata *ppd)
{
#if 0
	u64 reg;

	reg = read_csr(ppd->dd, FXR_ASIC_QSFP_IN);
	return !(reg & QSFP_HFI_MODPRST_N);
#endif
	if (test_qsfp) {
		/*
		 * FXRTODO: Returning true for Simics
		 * Remove this once hw is available
		 */
		return true;
	}
}

/*
 * This function maps QSFP memory addresses in 128 byte chunks in the following
 * fashion per the CableInfo SMA query definition in the IBA 1.3 spec/OPA Gen 1
 * spec
 * For addr 000-127, lower page 00h
 * For addr 128-255, upper page 00h
 * For addr 256-383, upper page 01h
 * For addr 384-511, upper page 02h
 * For addr 512-639, upper page 03h
 *
 * For addresses beyond this range, it returns the invalid range of data buffer
 * set to 0.
 * For upper pages that are optional, if they are not valid, returns the
 * particular range of bytes in the data buffer set to 0.
 */
int hfi_get_cable_info(struct hfi_devdata *dd, u32 port_num, u32 addr,
		       u32 len, u8 *data)
{
	struct hfi_pportdata *ppd;
	u32 excess_len = len;
	int ret = 0, offset = 0;

	if (port_num > dd->num_pports || port_num < 1) {
		dd_dev_info(dd, "%s: Invalid port number %d\n",
			    __func__, port_num);
		ret = -EINVAL;
		goto set_zeroes;
	}

	ppd = dd->pport + (port_num - 1);
	if (!hfi_qsfp_mod_present(ppd)) {
		ret = -ENODEV;
		goto set_zeroes;
	}

	if (test_qsfp) {
		/*
		 * FXRTODO: Remove the line to set the cache_valid variable to
		 * 1 and to clear the cache below once HW is available
		 */
		memset(&ppd->qsfp_info.cache, 0, QSFP_MAX_NUM_PAGES * 128);
		ppd->qsfp_info.cache_valid = 1;
	}

	if (!ppd->qsfp_info.cache_valid) {
		ret = -EINVAL;
		goto set_zeroes;
	}

	if (addr >= (QSFP_MAX_NUM_PAGES * 128)) {
		ret = -ERANGE;
		goto set_zeroes;
	}

	if ((addr + len) > (QSFP_MAX_NUM_PAGES * 128)) {
		excess_len = (addr + len) - (QSFP_MAX_NUM_PAGES * 128);
		memcpy(data, &ppd->qsfp_info.cache[addr], (len - excess_len));
		data += (len - excess_len);
		goto set_zeroes;
	}

	memcpy(data, &ppd->qsfp_info.cache[addr], len);

	if (addr <= QSFP_MONITOR_VAL_END &&
	    (addr + len) >= QSFP_MONITOR_VAL_START) {
		/* Overlap with the dynamic channel monitor range */
		if (addr < QSFP_MONITOR_VAL_START) {
			if (addr + len <= QSFP_MONITOR_VAL_END)
				len = addr + len - QSFP_MONITOR_VAL_START;
			else
				len = QSFP_MONITOR_RANGE;
			offset = QSFP_MONITOR_VAL_START - addr;
			addr = QSFP_MONITOR_VAL_START;
		} else if (addr == QSFP_MONITOR_VAL_START) {
			offset = 0;
			if (addr + len > QSFP_MONITOR_VAL_END)
				len = QSFP_MONITOR_RANGE;
		} else {
			offset = 0;
			if (addr + len > QSFP_MONITOR_VAL_END)
				len = QSFP_MONITOR_VAL_END - addr + 1;
		}
		/* Refresh the values of the dynamic monitors from the cable */
		ret = hfi_qsfp_read(ppd, dd->hfi_id, addr, data + offset, len);
		if (ret != len) {
			ret = -EAGAIN;
			goto set_zeroes;
		}
	}

	return 0;

set_zeroes:
	memset(data, 0, excess_len);
	return ret;
}

#if 0
static const char *pwr_codes[8] = {"N/AW",
				  "1.5W",
				  "2.0W",
				  "2.5W",
				  "3.5W",
				  "4.0W",
				  "4.5W",
				  "5.0W"
				 };

static int hfi_qsfp_dump(struct hfi_pportdata *ppd, char *buf, int len)
{
	u8 *cache = &ppd->qsfp_info.cache[0];
	u8 bin_buff[QSFP_DUMP_CHUNK];
	char lenstr[6];
	int sofar;
	int bidx = 0;
	u8 *atten = &cache[QSFP_ATTEN_OFFS];
	u8 *vendor_oui = &cache[QSFP_VOUI_OFFS];
	u8 power_byte = 0;

	sofar = 0;
	lenstr[0] = ' ';
	lenstr[1] = '\0';

	if (ppd->qsfp_info.cache_valid) {
		if (QSFP_IS_CU(cache[QSFP_MOD_TECH_OFFS]))
			snprintf(lenstr, sizeof(lenstr), "%dM ",
				 cache[QSFP_MOD_LEN_OFFS]);

		power_byte = cache[QSFP_MOD_PWR_OFFS];
		sofar += scnprintf(buf + sofar, len - sofar, "PWR:%.3sW\n",
			pwr_codes[hfi_get_qsfp_power_class(power_byte)]);

		sofar += scnprintf(buf + sofar, len - sofar, "TECH:%s%s\n",
			lenstr,
			hfi_qsfp_devtech[(cache[QSFP_MOD_TECH_OFFS]) >> 4]);

		sofar += scnprintf(buf + sofar, len - sofar, "Vendor:%.*s\n",
				   QSFP_VEND_LEN, &cache[QSFP_VEND_OFFS]);

		sofar += scnprintf(buf + sofar, len - sofar, "OUI:%06X\n",
				   QSFP_OUI(vendor_oui));

		sofar += scnprintf(buf + sofar, len - sofar, "Part#:%.*s\n",
				   QSFP_PN_LEN, &cache[QSFP_PN_OFFS]);

		sofar += scnprintf(buf + sofar, len - sofar, "Rev:%.*s\n",
				   QSFP_REV_LEN, &cache[QSFP_REV_OFFS]);

		if (QSFP_IS_CU(cache[QSFP_MOD_TECH_OFFS]))
			sofar += scnprintf(buf + sofar, len - sofar,
					   "Atten:%d, %d\n",
					   QSFP_ATTEN_SDR(atten),
					   QSFP_ATTEN_DDR(atten));

		sofar += scnprintf(buf + sofar, len - sofar, "Serial:%.*s\n",
				   QSFP_SN_LEN, &cache[QSFP_SN_OFFS]);

		sofar += scnprintf(buf + sofar, len - sofar, "Date:%.*s\n",
				   QSFP_DATE_LEN, &cache[QSFP_DATE_OFFS]);

		sofar += scnprintf(buf + sofar, len - sofar, "Lot:%.*s\n",
				   QSFP_LOT_LEN, &cache[QSFP_LOT_OFFS]);

		while (bidx < QSFP_DEFAULT_HDR_CNT) {
			int iidx;

			memcpy(bin_buff, &cache[bidx], QSFP_DUMP_CHUNK);
			for (iidx = 0; iidx < QSFP_DUMP_CHUNK; ++iidx) {
				sofar += scnprintf(buf + sofar, len - sofar,
						   " %02X", bin_buff[iidx]);
			}
			sofar += scnprintf(buf + sofar, len - sofar, "\n");
			bidx += QSFP_DUMP_CHUNK;
		}
	}
	return sofar;
}
#endif

void hfi_wait_for_qsfp_init(struct hfi_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;
	u64 mask;
	unsigned long timeout;

	/*
	 * Some QSFP cables have a quirk that asserts the IntN line as a side
	 * effect of power up on plug-in. We ignore this false positive
	 * interrupt until the module has finished powering up by waiting for
	 * a minimum timeout of the module inrush initialization time of
	 * 500 ms (SFF 8679 Table 5-6) to ensure the voltage rails in the
	 * module have stabilized.
	 */
	msleep(500);

	/*
	 * Check for QSFP interrupt for t_init (SFF 8679 Table 8-1)
	 */
	timeout = jiffies + msecs_to_jiffies(2000);
	while (1) {
		mask = read_csr(ppd->dd, FXR_ASIC_QSFP_IN);
		if (!(mask & QSFP_HFI_INT_N))
			break;
		if (time_after(jiffies, timeout)) {
			dd_dev_info(dd,
				    "%s: No IntN detected, reset complete\n",
				    __func__);
			break;
		}
		udelay(2);
	}
}

void hfi_set_qsfp_int_n(struct hfi_pportdata *ppd, u8 enable)
{
	u64 mask;

	mask = read_csr(ppd->dd, FXR_ASIC_QSFP_MASK);
	if (enable) {
		/*
		 * Clear the status register to avoid an immediate interrupt
		 * when we re-enable the IntN pin
		 */
		write_csr(ppd->dd, FXR_ASIC_QSFP_CLEAR, QSFP_HFI_INT_N);
		mask |= (u64)QSFP_HFI_INT_N;
	} else {
		mask &= ~(u64)QSFP_HFI_INT_N;
	}
	write_csr(ppd->dd, FXR_ASIC_QSFP_MASK, mask);
}

int hfi_reset_qsfp(struct hfi_pportdata *ppd)
{
	u64 mask, qsfp_mask;

	/* Disable INT_N from triggering QSFP interrupts */
	hfi_set_qsfp_int_n(ppd, 0);

	/* Reset the QSFP */
	mask = (u64)QSFP_HFI_RESET_N;

	qsfp_mask = read_csr(ppd->dd, FXR_ASIC_QSFP_OUT);
	qsfp_mask &= ~mask;
	write_csr(ppd->dd, FXR_ASIC_QSFP_OUT, qsfp_mask);

	usleep_range(10, 20);

	qsfp_mask |= mask;
	write_csr(ppd->dd, FXR_ASIC_QSFP_OUT, qsfp_mask);

	hfi_wait_for_qsfp_init(ppd);

	/*
	 * Allow INT_N to trigger the QSFP interrupt to watch
	 * for alarms and warnings
	 */
	hfi_set_qsfp_int_n(ppd, 1);

	/*
	 * After the reset, AOC transmitters are enabled by default.
	 * They need to be turned off to complete the QSFP setup before
	 * they can be enabled again
	 */
	return set_qsfp_tx(ppd, 0);
}

int hfi_set_qsfp_high_power(struct hfi_pportdata *ppd)
{
	u8 cable_power_class = 0, power_ctrl_byte = 0;
	u8 *cache = ppd->qsfp_info.cache;
	int ret;

	cable_power_class = hfi_get_qsfp_power_class(cache[QSFP_MOD_PWR_OFFS]);

	if (cable_power_class > QSFP_POWER_CLASS_1) {
		power_ctrl_byte = cache[QSFP_PWR_CTRL_BYTE_OFFS];

		power_ctrl_byte |= 1;
		power_ctrl_byte &= ~(0x2);

		ret = hfi_qsfp_write(ppd, ppd->dd->hfi_id,
				     QSFP_PWR_CTRL_BYTE_OFFS,
				     &power_ctrl_byte, 1);
		if (ret != 1)
			return -EIO;

		if (cable_power_class > QSFP_POWER_CLASS_4) {
			power_ctrl_byte |= (1 << 2);
			ret = hfi_qsfp_write(ppd, ppd->dd->hfi_id,
					     QSFP_PWR_CTRL_BYTE_OFFS,
					     &power_ctrl_byte, 1);
			if (ret != 1)
				return -EIO;
		}

		/* SFF 8679 rev 1.7 LPMode Deassert time */
		msleep(300);
	}
	return 0;
}

/*
 * Perform a test read on the QSFP.  Return 0 on success, -ERRNO
 * on error.
 */
int hfi_test_qsfp_read(struct hfi_pportdata *ppd)
{
	int ret;
	u8 status;

	/* report success if not a QSFP */
	if (ppd->port_type != PORT_TYPE_QSFP)
		return 0;

	/* read byte 2, the status byte */
	ret = hfi_qsfp_read(ppd, ppd->dd->hfi_id, 2, &status, 1);
	if (ret < 0)
		return ret;
	if (ret != 1)
		return -EIO;

	return 0; /* success */
}

void hfi_init_qsfp_int(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd = dd->pport;
	u64 qsfp_mask;

	qsfp_mask = (u64)(QSFP_HFI_INT_N | QSFP_HFI_MODPRST_N);
	/* Clear current status to avoid spurious interrupts */
	write_csr(dd, FXR_ASIC_QSFP_CLEAR, qsfp_mask);
	write_csr(dd, FXR_ASIC_QSFP_MASK, qsfp_mask);

	hfi_set_qsfp_int_n(ppd, 0);

	/* Handle active low nature of INT_N and MODPRST_N pins */
	if (hfi_qsfp_mod_present(ppd))
		qsfp_mask &= ~(u64)QSFP_HFI_MODPRST_N;
	write_csr(dd, FXR_ASIC_QSFP_INVERT, qsfp_mask);
}

/*
 * This routine will only be scheduled if the QSFP module present is asserted
 */
void hfi_qsfp_event(struct work_struct *work)
{
	struct qsfp_data *qd;
	struct hfi_pportdata *ppd;
	struct hfi_devdata *dd;

	qd = container_of(work, struct qsfp_data, qsfp_work);
	ppd = qd->ppd;
	dd = ppd->dd;

	/* Sanity check */
	if (!hfi_qsfp_mod_present(ppd))
		return;

	/*
	 * Turn MNH back on after cable has been re-inserted. Up until
	 * now, the MNH has been in reset to save power.
	 */
	oc_start(ppd);

	if (qd->cache_refresh_required) {
		hfi_set_qsfp_int_n(ppd, 0);

		hfi_wait_for_qsfp_init(ppd);

		/*
		 * Allow INT_N to trigger the QSFP interrupt to watch
		 * for alarms and warnings
		 */
		hfi_set_qsfp_int_n(ppd, 1);

		hfi_start_link(ppd);
	}

	if (qd->check_interrupt_flags) {
		u8 qsfp_interrupt_status[16] = {0,};

		if (hfi_qsfp_read(ppd, dd->hfi_id, 6,
				  &qsfp_interrupt_status[0], 16) != 16) {
			dd_dev_info(
				dd,
				"%s: Failed to read status of QSFP module\n",
				__func__);
		} else {
			unsigned long flags;

			hfi_handle_qsfp_error_conditions(
				ppd, qsfp_interrupt_status);
			spin_lock_irqsave(&ppd->qsfp_info.qsfp_lock, flags);
			ppd->qsfp_info.check_interrupt_flags = 0;
			spin_unlock_irqrestore(&ppd->qsfp_info.qsfp_lock,
					       flags);
		}
	}
}
