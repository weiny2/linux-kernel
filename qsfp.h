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
/* QSFP support common definitions, for hfi driver */

#define QSFP_DEV 0xA0
#define QSFP_PWR_LAG_MSEC 2000
#define QSFP_MODPRS_LAG_MSEC 20
/* 128 byte pages, per SFF 8636 rev 2.4 */
#define QSFP_MAX_NUM_PAGES	4

/*
 * Below are masks for QSFP pins.  Pins are the same for HFI0 and HFI1.
 * _N means asserted low
 */
#define QSFP_HFI0_I2CCLK    (1 << 0)
#define QSFP_HFI0_I2CDAT    (1 << 1)
#define QSFP_HFI0_RESET_N   (1 << 2)
#define QSFP_HFI0_INT_N	    (1 << 3)
#define QSFP_HFI0_MODPRST_N (1 << 4)

/* QSFP is paged at 256 bytes */
#define QSFP_PAGESIZE 256

/* Defined fields that QLogic requires of qualified cables */
/* Byte 0 is Identifier, not checked */
/* Byte 1 is reserved "status MSB" */
/* Byte 2 is "status LSB" We only care that D2 "Flat Mem" is set. */
/*
 * Rest of first 128 not used, although 127 is reserved for page select
 * if module is not "Flat memory".
 */
#define QSFP_PAGE_SELECT_BYTE_OFFS 127
/* Byte 128 is Identifier: must be 0x0c for QSFP, or 0x0d for QSFP+ */
#define QSFP_MOD_ID_OFFS 128
/*
 * Byte 129 is "Extended Identifier". We only care about D7,D6: Power class
 *  0:1.5W, 1:2.0W, 2:2.5W, 3:3.5W
 */
#define QSFP_MOD_PWR_OFFS 129
/* Byte 130 is Connector type. Not QLogic req'd */
/* Bytes 131..138 are Transceiver types, bit maps for various tech, none IB */
/* Byte 139 is encoding. code 0x01 is 8b10b. Not QLogic req'd */
/* byte 140 is nominal bit-rate, in units of 100Mbits/sec Not QLogic req'd */
/* Byte 141 is Extended Rate Select. Not QLogic req'd */
/* Bytes 142..145 are lengths for various fiber types. Not QLogic req'd */
/* Byte 146 is length for Copper. Units of 1 meter */
#define QSFP_MOD_LEN_OFFS 146
/*
 * Byte 147 is Device technology. D0..3 not Qlogc req'd
 * D4..7 select from 15 choices, translated by table:
 */
#define QSFP_MOD_TECH_OFFS 147
extern const char *const qib_qsfp_devtech[16];
/* Active Equalization includes fiber, copper full EQ, and copper near Eq */
#define QSFP_IS_ACTIVE(tech) ((0xA2FF >> ((tech) >> 4)) & 1)
/* Active Equalization includes fiber, copper full EQ, and copper far Eq */
#define QSFP_IS_ACTIVE_FAR(tech) ((0x32FF >> ((tech) >> 4)) & 1)
/* Attenuation should be valid for copper other than full/near Eq */
#define QSFP_HAS_ATTEN(tech) ((0x4D00 >> ((tech) >> 4)) & 1)
/* Length is only valid if technology is "copper" */
#define QSFP_IS_CU(tech) ((0xED00 >> ((tech) >> 4)) & 1)
#define QSFP_TECH_1490 9

#define QSFP_OUI(oui) (((unsigned)oui[0] << 16) | ((unsigned)oui[1] << 8) | \
			oui[2])
#define QSFP_OUI_AMPHENOL 0x415048
#define QSFP_OUI_FINISAR  0x009065
#define QSFP_OUI_GORE     0x002177

/* Bytes 148..163 are Vendor Name, Left-justified Blank-filled */
#define QSFP_VEND_OFFS 148
#define QSFP_VEND_LEN 16
/* Byte 164 is IB Extended tranceiver codes Bits D0..3 are SDR,DDR,QDR,EDR */
#define QSFP_IBXCV_OFFS 164
/* Bytes 165..167 are Vendor OUI number */
#define QSFP_VOUI_OFFS 165
#define QSFP_VOUI_LEN 3
/* Bytes 168..183 are Vendor Part Number, string */
#define QSFP_PN_OFFS 168
#define QSFP_PN_LEN 16
/* Bytes 184,185 are Vendor Rev. Left Justified, Blank-filled */
#define QSFP_REV_OFFS 184
#define QSFP_REV_LEN 2
/*
 * Bytes 186,187 are Wavelength, if Optical. Not Qlogic req'd
 *  If copper, they are attenuation in dB:
 * Byte 186 is at 2.5Gb/sec (SDR), Byte 187 at 5.0Gb/sec (DDR)
 */
#define QSFP_ATTEN_OFFS 186
#define QSFP_ATTEN_LEN 2
/* Bytes 188,189 are Wavelength tolerance, not QLogic req'd */
/* Byte 190 is Max Case Temp. Not QLogic req'd */
/* Byte 191 is LSB of sum of bytes 128..190. Not QLogic req'd */
#define QSFP_CC_OFFS 191
/* Bytes 192..195 are Options implemented in qsfp. Not Qlogic req'd */
/* Bytes 196..211 are Serial Number, String */
#define QSFP_SN_OFFS 196
#define QSFP_SN_LEN 16
/* Bytes 212..219 are date-code YYMMDD (MM==1 for Jan) */
#define QSFP_DATE_OFFS 212
#define QSFP_DATE_LEN 6
/* Bytes 218,219 are optional lot-code, string */
#define QSFP_LOT_OFFS 218
#define QSFP_LOT_LEN 2
/* Bytes 220, 221 indicate monitoring options, Not QLogic req'd */
/* Byte 223 is LSB of sum of bytes 192..222 */
#define QSFP_CC_EXT_OFFS 223

/*
 * Interrupt flag masks
 */
#define QSFP_DATA_NOT_READY		0x01

#define QSFP_HIGH_TEMP_ALARM		0x80
#define QSFP_LOW_TEMP_ALARM		0x40
#define QSFP_HIGH_TEMP_WARNING		0x20
#define QSFP_LOW_TEMP_WARNING		0x10

#define QSFP_HIGH_VCC_ALARM		0x80
#define QSFP_LOW_VCC_ALARM		0x40
#define QSFP_HIGH_VCC_WARNING		0x20
#define QSFP_LOW_VCC_WARNING		0x10

#define QSFP_HIGH_POWER_ALARM		0x88
#define QSFP_LOW_POWER_ALARM		0x44
#define QSFP_HIGH_POWER_WARNING		0x22
#define QSFP_LOW_POWER_WARNING		0x11

#define QSFP_HIGH_BIAS_ALARM		0x88
#define QSFP_LOW_BIAS_ALARM		0x44
#define QSFP_HIGH_BIAS_WARNING		0x22
#define QSFP_LOW_BIAS_WARNING		0x11

/*
 * struct qsfp_data encapsulates state of QSFP device for one port.
 * it will be part of port-specific data if a board supports QSFP.
 *
 * Since multiple board-types use QSFP, and their pport_data structs
 * differ (in the chip-specific section), we need a pointer to its head.
 *
 * Avoiding premature optimization, we will have one work_struct per port,
 * and let the qsfp_lock arbitrate access to common resources.
 *
 */

#define QSFP_PWR(pbyte) (((pbyte) >> 6) & 3)
#define QSFP_ATTEN_SDR(attenarray) (attenarray[0])
#define QSFP_ATTEN_DDR(attenarray) (attenarray[1])

struct qsfp_data {
	/* Helps to find our way */
	struct qib_pportdata *ppd;
	struct work_struct qsfp_work;
	u8 cache[QSFP_MAX_NUM_PAGES*128];
	spinlock_t qsfp_lock;
	u8 check_interrupt_flags;
	u8 qsfp_interrupt_functional;
	u8 cache_valid;
	u8 cache_refresh_required;
};

extern int refresh_qsfp_cache(struct qib_pportdata *ppd,
				  struct qsfp_data *cp);
extern int qsfp_mod_present(struct qib_pportdata *ppd);
extern int get_cable_info(struct hfi_devdata *dd, u32 port_num, u32 addr,
			  u32 len, u8 *data);
extern void qib_qsfp_init(struct qib_pportdata *ppd);

extern int i2c_write(struct qib_pportdata *ppd, u32 target, int i2c_addr,
		     int offset, void *bp, int len);
extern int i2c_read(struct qib_pportdata *ppd, u32 target, int i2c_addr,
		    int offset, void *bp, int len);
extern int qsfp_write(struct qib_pportdata *ppd, u32 target, int addr, void *bp,
		      int len);
extern int qsfp_read(struct qib_pportdata *ppd, u32 target, int addr, void *bp,
		     int len);
