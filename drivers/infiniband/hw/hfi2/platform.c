/*
 * Copyright(c) 2015, 2016 Intel Corporation.
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

#include <linux/firmware.h>
#include <linux/crc32.h>
#include "hfi2.h"
#include "link.h"
#include "chip/fxr_top_defs.h"
#include "chip/fxr_linkmux_defs.h"

/* Size of the platform config partition */
#define MAX_PLATFORM_CONFIG_FILE_SIZE  4096

/* size of the file of platform configuration encoded in format version 4 */
#define PLATFORM_CONFIG_FORMAT_4_FILE_SIZE     528

static char *platform_config_name = DEFAULT_PLATFORM_CONFIG_NAME;

static const u32 platform_config_table_limits[PLATFORM_CONFIG_TABLE_MAX] = {
	0,
	SYSTEM_TABLE_MAX,
	PORT_TABLE_MAX,
	RX_PRESET_TABLE_MAX,
	TX_PRESET_TABLE_MAX,
	QSFP_ATTEN_TABLE_MAX,
	VARIABLE_SETTINGS_TABLE_MAX
};

static int validate_scratch_checksum(struct hfi_devdata *dd)
{
	u64 checksum = 0, tmp_scratch = 0;
	int i, j, version;

	tmp_scratch = read_csr(dd, FXR_LM_FXR_SCRATCH);
	version = (tmp_scratch & BITMAP_VERSION_SMASK) >> BITMAP_VERSION_SHIFT;

	/* Prevent power on default of all zeroes from passing checksum */
	if (!version) {
		dd_dev_err(dd, "%s: Config bitmap uninitialized\n", __func__);
		dd_dev_err(dd,
			   "%s: Please update your BIOS to support active channels\n",
			   __func__);
		return 0;
	}

	/*
	 * LM_SCRATCH_0 only contains the checksum and bitmap version as
	 * fields of interest, both of which are handled separately from
	 * the loop below, so skip it
	 */
	checksum += version;
	for (i = 1; i < NUM_QSFP_SCRATCH; i++) {
		tmp_scratch = read_csr(dd, FXR_LM_FXR_SCRATCH + (8 * i));
		for (j = sizeof(u64); j != 0; j -= 2) {
			checksum += (tmp_scratch & 0xFFFF);
			tmp_scratch >>= 16;
		}
	}

	while (checksum >> 16)
		checksum = (checksum & CHECKSUM_MASK) + (checksum >> 16);

	tmp_scratch = read_csr(dd, FXR_LM_FXR_SCRATCH);
	tmp_scratch &= CHECKSUM_MASK;
	tmp_scratch >>= CHECKSUM_SHIFT;

	if (checksum + tmp_scratch == 0xFFFF)
		return 1;

	dd_dev_err(dd, "%s: Configuration bitmap corrupted\n", __func__);
	return 0;
}

static void save_platform_config_fields(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd = dd->pport;
	u64 tmp_scratch = 0, tmp_dest = 0;

	/* Read scratch register 1. Hence, the 8 byte offset */
	tmp_scratch = read_csr(dd, FXR_LM_FXR_SCRATCH + 8);

	/*
	 * FXRTODO: Remove the #if 0, #endif once QSFP hardware is
	 * available. Currently the port_type is hard coded to QSFP
	 * inside hfi_pport_init();
	 */
#if 0
	tmp_dest = tmp_scratch & PORT_TYPE_SMASK;
	ppd->port_type = tmp_dest >> PORT_TYPE_SHIFT;
#endif
	tmp_dest = tmp_scratch & LOCAL_ATTEN_SMASK;
	ppd->local_atten = tmp_dest >> LOCAL_ATTEN_SHIFT;

	tmp_dest = tmp_scratch & REMOTE_ATTEN_SMASK;
	ppd->remote_atten = tmp_dest >> REMOTE_ATTEN_SHIFT;

	tmp_dest = tmp_scratch & DEFAULT_ATTEN_SMASK;
	ppd->default_atten = tmp_dest >> DEFAULT_ATTEN_SHIFT;

	/* Read scratch register 2. Hence, the 16 byte offset */
	tmp_scratch = read_csr(dd, FXR_LM_FXR_SCRATCH + 16);

	ppd->tx_preset_eq = (tmp_scratch & TX_EQ_SMASK) >> TX_EQ_SHIFT;
	ppd->tx_preset_noeq = (tmp_scratch & TX_NO_EQ_SMASK) >> TX_NO_EQ_SHIFT;
	ppd->rx_preset = (tmp_scratch & RX_SMASK) >> RX_SHIFT;

	ppd->max_power_class = (tmp_scratch & QSFP_MAX_POWER_SMASK) >>
				QSFP_MAX_POWER_SHIFT;
}

void hfi2_free_platform_config(struct hfi_devdata *dd)
{
	switch (dd->platform_config.from) {
	case NOT_READ_YET:
		/* fallthrough */
	case FROM_SCRATCH_CSR:
		/* do nothing */
		break;
	case FROM_LIB_FIRMWARE:
		release_firmware(dd->platform_config.firmware);
		break;
	default:
		dd_dev_warn(dd,
			    "%s %d: don't know where platform_config is read from: %d\n",
			    __func__, __LINE__, dd->platform_config.from);
		break;
	}
}

/*
 * read platform_config file
 */
void hfi2_get_platform_config(struct hfi_devdata *dd)
{
	int ret = 0;

	dd->platform_config.from = NOT_READ_YET;
	dd->platform_config.firmware = NULL;
	dd->platform_config.data = NULL;
	dd->platform_config.size = 0;

	if (validate_scratch_checksum(dd)) {
		dd->platform_config.from = FROM_SCRATCH_CSR;
		save_platform_config_fields(dd);
		return;
	}
	ret = request_firmware(&dd->platform_config.firmware,
			       platform_config_name, &dd->pdev->dev);
	if (ret == 0) {
		dd->platform_config.from = FROM_LIB_FIRMWARE;
		dd->platform_config.data =
			dd->platform_config.firmware->data;
		dd->platform_config.size =
			dd->platform_config.firmware->size;
	} else { /* can't read from both */
		dd_dev_warn(dd, "%s %d : can't read: %s\n",
			    __func__, __LINE__, platform_config_name);
	}
}

/*
 * This function is a helper function for hfi2_parse_platform_config(...) and
 * does not check for validity of the platform configuration cache
 * (because we know it is invalid as we are building up the cache).
 * As such, this should not be called from anywhere other than
 * hfi2_parse_platform_config
 */
static int check_meta_version(struct hfi_devdata *dd, u32 *system_table)
{
	u32 meta_ver, meta_ver_meta, ver_start, ver_len, mask;
	struct platform_config_cache *pcfgcache = &dd->pcfg_cache;

	if (!system_table)
		return -EINVAL;

	meta_ver_meta =
	*(pcfgcache->config_tables[PLATFORM_CONFIG_SYSTEM_TABLE].table_metadata
	+ SYSTEM_TABLE_META_VERSION);

	mask = ((1 << METADATA_TABLE_FIELD_START_LEN_BITS) - 1);
	ver_start = meta_ver_meta & mask;

	meta_ver_meta >>= METADATA_TABLE_FIELD_LEN_SHIFT;

	mask = ((1 << METADATA_TABLE_FIELD_START_LEN_BITS) - 1);
	ver_len = meta_ver_meta & mask;

	ver_start /= 8;
	meta_ver = *((u8 *)system_table + ver_start) & ((1 << ver_len) - 1);

	if (meta_ver < 4) {
		dd_dev_info(
			dd, "%s:Please update platform config\n", __func__);
		return -EINVAL;
	}
	return 0;
}

/*
 * dd->platform_config -> dd->pcfg_cache
 */
int hfi2_parse_platform_config(struct hfi_devdata *dd)
{
	struct platform_config_cache *pcfgcache = &dd->pcfg_cache;
	u32 *ptr = NULL;
	u32 header1 = 0, header2 = 0, magic_num = 0, crc = 0, file_length = 0;
	u32 record_idx = 0, table_type = 0, table_length_dwords = 0;
	int ret = -EINVAL; /* assume failure */

	/*
	 * For devices that did not fall back to the default file, the
	 * SI tuning information for active channels is acquired from
	 * the scratch register bitmap, thus there is no platform config
	 * to parse. Skip parsing in these situations
	 */
	if (dd->platform_config.from != FROM_LIB_FIRMWARE)
		return 0;

	if (!dd->platform_config.data) {
		dd_dev_warn(dd, "%s %d: Missing config file\n",
			    __func__, __LINE__);
		goto bail;
	}
	ptr = (u32 *)dd->platform_config.data;

	magic_num = *ptr;
	ptr++;
	if (magic_num != PLATFORM_CONFIG_MAGIC_NUM) {
		dd_dev_warn(dd, "%s %d: Bad config file: 0x%x expect 0x%x\n",
			    __func__, __LINE__, magic_num,
			    PLATFORM_CONFIG_MAGIC_NUM);
		goto bail;
	}

	/* Field is file size in DWORDs */
	file_length = (*ptr) * 4;

	/*
	 * Length can't be larget than the partition size. Assume
	 * platform config format version 4 being used. Interpret the
	 * file size field as the header instead by not moving the
	 * pointer.
	 */
	if (file_length > MAX_PLATFORM_CONFIG_FILE_SIZE) {
		dd_dev_info(dd,
			    "%s: File length out of bounds, using alternate format\n",
			     __func__);
		file_length = PLATFORM_CONFIG_FORMAT_4_FILE_SIZE;

	} else {
		ptr++;
	}

	if (file_length > dd->platform_config.size) {
		dd_dev_info(dd, "%s: File claims to be larger than read size\n",
			    __func__);
		goto bail;
	} else if (file_length < dd->platform_config.size) {
		dd_dev_info(dd,
			    "%s: File claims to be smaller than read size, continuing\n",
			    __func__);
	}
	/* exactly equal, perfection */

	/*
	 * In both cases where we proceed, using the self-reported file length
	 * is the safer option. In case of old format, a prefdefined
	 * value is being used.
	 */
	while (ptr < (u32 *)(dd->platform_config.data + file_length)) {
		header1 = *ptr;
		header2 = *(ptr + 1);
		if (header1 != ~header2) {
			dd_dev_info(dd, "%s: Failed validation at offset %ld\n",
				    __func__, (ptr -
					      (u32 *)dd->platform_config.data));
			goto bail;
		}

		record_idx = *ptr &
			((1 << PLATFORM_CONFIG_HEADER_RECORD_IDX_LEN_BITS) - 1);

		table_length_dwords = (*ptr >>
				PLATFORM_CONFIG_HEADER_TABLE_LENGTH_SHIFT) &
		      ((1 << PLATFORM_CONFIG_HEADER_TABLE_LENGTH_LEN_BITS) - 1);

		table_type = (*ptr >> PLATFORM_CONFIG_HEADER_TABLE_TYPE_SHIFT) &
			((1 << PLATFORM_CONFIG_HEADER_TABLE_TYPE_LEN_BITS) - 1);

		/* Done with this set of headers */
		ptr += 2;

		if (record_idx) {
			/* data table */
			switch (table_type) {
			case PLATFORM_CONFIG_SYSTEM_TABLE:
				pcfgcache->config_tables[table_type].num_table =
									1;
				ret = check_meta_version(dd, ptr);
				if (ret)
					goto bail;
				break;
			case PLATFORM_CONFIG_PORT_TABLE:
				pcfgcache->config_tables[table_type].num_table =
									2;
				break;
			case PLATFORM_CONFIG_RX_PRESET_TABLE:
				/* fall through */
			case PLATFORM_CONFIG_TX_PRESET_TABLE:
				/* fall through */
			case PLATFORM_CONFIG_QSFP_ATTEN_TABLE:
				/* fall through */
			case PLATFORM_CONFIG_VARIABLE_SETTINGS_TABLE:
				pcfgcache->config_tables[table_type].num_table =
							table_length_dwords;
				break;
			default:
				dd_dev_info(dd,
					    "%s: Unknown data table %d, offset %ld\n",
					    __func__, table_type,
					    (ptr -
					    (u32 *)dd->platform_config.data));
				goto bail; /* We don't trust this file now */
			}
			pcfgcache->config_tables[table_type].table = ptr;
		} else {
			/* metadata table */
			switch (table_type) {
			case PLATFORM_CONFIG_SYSTEM_TABLE:
				/* fall through */
			case PLATFORM_CONFIG_PORT_TABLE:
				/* fall through */
			case PLATFORM_CONFIG_RX_PRESET_TABLE:
				/* fall through */
			case PLATFORM_CONFIG_TX_PRESET_TABLE:
				/* fall through */
			case PLATFORM_CONFIG_QSFP_ATTEN_TABLE:
				/* fall through */
			case PLATFORM_CONFIG_VARIABLE_SETTINGS_TABLE:
				break;
			default:
				dd_dev_info(dd,
					    "%s: Unknown data table %d, offset %ld\n",
					    __func__, table_type,
					    (ptr -
					    (u32 *)dd->platform_config.data));
				goto bail; /* We don't trust this file now */
			}
			pcfgcache->config_tables[table_type].table_metadata =
									ptr;
		}

		/* Calculate and check table crc */
		crc = crc32_le(~(u32)0, (unsigned char const *)ptr,
			       (table_length_dwords * 4));
		crc ^= ~(u32)0;

		/* Jump the table */
		ptr += table_length_dwords;
		if (crc != *ptr) {
			dd_dev_info(dd, "%s: Failed CRC check at offset %ld\n",
				    __func__, (ptr -
					      (u32 *)dd->platform_config.data));
			goto bail;
		}
		/* Jump the CRC DWORD */
		ptr++;
	}

	pcfgcache->cache_valid = 1;
	return 0;
bail:
	memset(pcfgcache, 0, sizeof(struct platform_config_cache));
	return ret;
}

static int qual_bitrate(struct hfi_pportdata *ppd)
{
	u16 lss = ppd->link_speed_supported, lse = ppd->link_speed_enabled;
	u8 *cache = ppd->qsfp_info.cache;

	if ((lss & OPA_LINK_SPEED_25G) && (lse & OPA_LINK_SPEED_25G) &&
	    cache[QSFP_NOM_BIT_RATE_250_OFFS] < 0x64) {
		ppd->offline_disabled_reason =
			   HFI_ODR_MASK(OPA_LINKDOWN_REASON_LINKSPEED_POLICY);
		dd_dev_err(ppd->dd,
			    "%s: Cable failed bitrate check, disabling port\n",
			    __func__);
		return -EPERM;
	}
	return 0;
}

static void apply_rx_cdr(struct hfi_pportdata *ppd, u32 rx_preset_index,
			 u8 *cdr_ctrl_byte)
{
	u32 rx_preset;
	u8 *cache = ppd->qsfp_info.cache;
	int cable_power_class;

	if (!((cache[QSFP_MOD_PWR_OFFS] & 0x4) &&
	      (cache[QSFP_CDR_INFO_OFFS] & 0x40)))
		return;

	/* RX CDR present, bypass supported */
	cable_power_class = hfi_get_qsfp_power_class(cache[QSFP_MOD_PWR_OFFS]);
#if 0
	/*
	 * FXRTODO: Check if the SerDes requires it and whether the KNH
	 * boards can cool the higher power consumption
	 */
	if (cable_power_class <= QSFP_POWER_CLASS_3) {
		/* Power class <= 3, ignore config & turn RX CDR on */
		*cdr_ctrl_byte |= 0xF;
		return;
	}
#endif
	get_platform_config_field(ppd->dd, PLATFORM_CONFIG_RX_PRESET_TABLE,
				  rx_preset_index,
				  RX_PRESET_TABLE_QSFP_RX_CDR_APPLY,
				  &rx_preset, 4);

	if (!rx_preset) {
		dd_dev_info(ppd->dd, "%s: RX_CDR_APPLY is set to disabled\n",
			    __func__);
		return;
	}

	get_platform_config_field(ppd->dd, PLATFORM_CONFIG_RX_PRESET_TABLE,
				  rx_preset_index, RX_PRESET_TABLE_QSFP_RX_CDR,
				  &rx_preset, 4);

	/* Expand cdr setting to all 4 lanes */
	rx_preset = (rx_preset | (rx_preset << 1) |
		    (rx_preset << 2) | (rx_preset << 3));

	if (rx_preset)
		*cdr_ctrl_byte |= rx_preset;
	else
		/* Preserve current TX CDR status */
		*cdr_ctrl_byte = (cache[QSFP_CDR_CTRL_BYTE_OFFS] & 0xF0);
}

static void apply_tx_cdr(struct hfi_pportdata *ppd, u32 tx_preset_index,
			 u8 *cdr_ctrl_byte)
{
	u32 tx_preset;
	u8 *cache = ppd->qsfp_info.cache;
	int cable_power_class;

	if (!((cache[QSFP_MOD_PWR_OFFS] & 0x8) &&
	      (cache[QSFP_CDR_INFO_OFFS] & 0x80)))
		return;

	/* TX CDR present, bypass supported */
	cable_power_class = hfi_get_qsfp_power_class(cache[QSFP_MOD_PWR_OFFS]);
#if 0
	/*
	 * FXRTODO: Check if the SerDes requires it and whether the KNH
	 * boards can cool the higher power consumption
	 */
	if (cable_power_class <= QSFP_POWER_CLASS_3) {
		/* Power class <= 3, ignore config & turn TX CDR on */
		*cdr_ctrl_byte |= 0xF0;
		return;
	}
#endif
	get_platform_config_field(ppd->dd, PLATFORM_CONFIG_TX_PRESET_TABLE,
				  tx_preset_index,
				  TX_PRESET_TABLE_QSFP_TX_CDR_APPLY,
				  &tx_preset, 4);

	if (!tx_preset) {
		dd_dev_info(ppd->dd, "%s: TX_CDR_APPLY is set to disabled\n",
			    __func__);
		return;
	}

	get_platform_config_field(ppd->dd, PLATFORM_CONFIG_TX_PRESET_TABLE,
				  tx_preset_index, TX_PRESET_TABLE_QSFP_TX_CDR,
				  &tx_preset, 4);

	/* Expand cdr setting to all 4 lanes */
	tx_preset = (tx_preset | (tx_preset << 1) |
		    (tx_preset << 2) | (tx_preset << 3));

	if (tx_preset)
		*cdr_ctrl_byte |= (tx_preset << 4);
	else
		/* Preserve current/determined RX CDR status */
		*cdr_ctrl_byte &= 0xF;
}

static void apply_cdr_settings(struct hfi_pportdata *ppd, u32 rx_preset_index,
			       u32 tx_preset_index)
{
	u8 *cache = ppd->qsfp_info.cache;
	u8 cdr_ctrl_byte = cache[QSFP_CDR_CTRL_BYTE_OFFS];

	apply_rx_cdr(ppd, rx_preset_index, &cdr_ctrl_byte);

	apply_tx_cdr(ppd, tx_preset_index, &cdr_ctrl_byte);

	hfi_qsfp_write(ppd, ppd->dd->hfi_id, QSFP_CDR_CTRL_BYTE_OFFS,
		       &cdr_ctrl_byte, 1);
}

static void apply_tx_eq_auto(struct hfi_pportdata *ppd)
{
	u8 *cache = ppd->qsfp_info.cache;
	u8 tx_eq;

	if (!(cache[QSFP_EQ_INFO_OFFS] & 0x8))
		return;
	/* Disable adaptive TX EQ if present */
	tx_eq = cache[(128 * 3) + 241];
	tx_eq &= 0xF0;
	hfi_qsfp_write(ppd, ppd->dd->hfi_id, (256 * 3) + 241, &tx_eq, 1);
}

static void apply_tx_eq_prog(struct hfi_pportdata *ppd, u32 tx_preset_index)
{
	u8 *cache = ppd->qsfp_info.cache;
	u32 tx_preset;
	u8 tx_eq;

	if (!(cache[QSFP_EQ_INFO_OFFS] & 0x4))
		return;

	get_platform_config_field(ppd->dd, PLATFORM_CONFIG_TX_PRESET_TABLE,
				  tx_preset_index,
				  TX_PRESET_TABLE_QSFP_TX_EQ_APPLY,
				  &tx_preset, 4);
	if (!tx_preset) {
		dd_dev_info(ppd->dd, "%s: TX_EQ_APPLY is set to disabled\n",
			    __func__);
		return;
	}
	get_platform_config_field(ppd->dd, PLATFORM_CONFIG_TX_PRESET_TABLE,
				  tx_preset_index, TX_PRESET_TABLE_QSFP_TX_EQ,
				  &tx_preset, 4);

	if (((cache[(128 * 3) + 224] & 0xF0) >> 4) < tx_preset) {
		dd_dev_info(ppd->dd, "%s: TX EQ %x unsupported\n",
			    __func__, tx_preset);

		dd_dev_info(ppd->dd, "%s: Applying EQ %x\n",
			    __func__, cache[608] & 0xF0);

		tx_preset = (cache[608] & 0xF0) >> 4;
	}

	tx_eq = tx_preset | (tx_preset << 4);
	hfi_qsfp_write(ppd, ppd->dd->hfi_id, (256 * 3) + 234, &tx_eq, 1);
	hfi_qsfp_write(ppd, ppd->dd->hfi_id, (256 * 3) + 235, &tx_eq, 1);
}

static void apply_rx_eq_emp(struct hfi_pportdata *ppd, u32 rx_preset_index)
{
	u32 rx_preset;
	u8 rx_eq, *cache = ppd->qsfp_info.cache;

	if (!(cache[QSFP_EQ_INFO_OFFS] & 0x2))
		return;
	get_platform_config_field(ppd->dd, PLATFORM_CONFIG_RX_PRESET_TABLE,
				  rx_preset_index,
				  RX_PRESET_TABLE_QSFP_RX_EMP_APPLY,
				  &rx_preset, 4);

	if (!rx_preset) {
		dd_dev_info(ppd->dd, "%s: RX_EMP_APPLY is set to disabled\n",
			    __func__);
		return;
	}
	get_platform_config_field(ppd->dd, PLATFORM_CONFIG_RX_PRESET_TABLE,
				  rx_preset_index, RX_PRESET_TABLE_QSFP_RX_EMP,
				  &rx_preset, 4);

	if ((cache[(128 * 3) + 224] & 0xF) < rx_preset) {
		dd_dev_info(ppd->dd, "%s: Requested RX EMP %x\n",
			    __func__, rx_preset);

		dd_dev_info(ppd->dd, "%s: Applying supported EMP %x\n",
			    __func__, cache[608] & 0xF);

		rx_preset = cache[608] & 0xF;
	}

	rx_eq = rx_preset | (rx_preset << 4);

	hfi_qsfp_write(ppd, ppd->dd->hfi_id, (256 * 3) + 236, &rx_eq, 1);
	hfi_qsfp_write(ppd, ppd->dd->hfi_id, (256 * 3) + 237, &rx_eq, 1);
}

static void apply_eq_settings(struct hfi_pportdata *ppd, u32 rx_preset_index,
			      u32 tx_preset_index)
{
	u8 *cache = ppd->qsfp_info.cache;

	/* no point going on w/o a page 3 */
	if (cache[2] & 4) {
		dd_dev_info(ppd->dd, "%s: Upper page 03 not present\n",
			    __func__);
		return;
	}

	apply_tx_eq_auto(ppd);

	apply_tx_eq_prog(ppd, tx_preset_index);

	apply_rx_eq_emp(ppd, rx_preset_index);
}

static void apply_rx_amplitude_settings(struct hfi_pportdata *ppd,
					u32 rx_preset_index,
					u32 tx_preset_index)
{
	u32 rx_preset;
	u8 rx_amp = 0, i = 0, preferred = 0, *cache = ppd->qsfp_info.cache;

	/* no point going on w/o a page 3 */
	if (cache[2] & 4) {
		dd_dev_info(ppd->dd, "%s: Upper page 03 not present\n",
			    __func__);
		return;
	}
	if (!(cache[QSFP_EQ_INFO_OFFS] & 0x1)) {
		dd_dev_info(ppd->dd, "%s: RX_AMP_APPLY is set to disabled\n",
			    __func__);
		return;
	}

	get_platform_config_field(ppd->dd,
				  PLATFORM_CONFIG_RX_PRESET_TABLE,
				  rx_preset_index,
				  RX_PRESET_TABLE_QSFP_RX_AMP_APPLY,
				  &rx_preset, 4);

	if (!rx_preset) {
		dd_dev_info(ppd->dd, "%s: RX_AMP_APPLY is set to disabled\n",
			    __func__);
		return;
	}
	get_platform_config_field(ppd->dd, PLATFORM_CONFIG_RX_PRESET_TABLE,
				  rx_preset_index, RX_PRESET_TABLE_QSFP_RX_AMP,
				  &rx_preset, 4);

	dd_dev_info(ppd->dd, "%s: Requested RX AMP %x\n", __func__, rx_preset);

	for (i = 0; i < 4; i++) {
		if (cache[(128 * 3) + 225] & (1 << i)) {
			preferred = i;
			if (preferred == rx_preset)
				break;
		}
	}

	/*
	 * Verify that preferred RX amplitude is not just a
	 * fall through of the default
	 */
	if (!preferred && !(cache[(128 * 3) + 225] & 0x1)) {
		dd_dev_info(ppd->dd, "No supported RX AMP, not applying\n");
		return;
	}

	dd_dev_info(ppd->dd, "%s: Applying RX AMP %x\n", __func__, preferred);

	rx_amp = preferred | (preferred << 4);
	hfi_qsfp_write(ppd, ppd->dd->hfi_id, (256 * 3) + 238, &rx_amp, 1);
	hfi_qsfp_write(ppd, ppd->dd->hfi_id, (256 * 3) + 239, &rx_amp, 1);
}

int set_qsfp_tx(struct hfi_pportdata *ppd, int on)
{
	u8 tx_ctrl_byte = on ? 0x0 : 0xF;
	int ret = 0;

	ret = hfi_qsfp_write(ppd, ppd->dd->hfi_id, QSFP_TX_CTRL_BYTE_OFFS,
			     &tx_ctrl_byte, 1);
	/* we expected 1, so consider 0 an error */
	if (ret == 0)
		ret = -EIO;
	else if (ret == 1)
		ret = 0;
	return ret;
}

static int qual_power(struct hfi_pportdata *ppd)
{
	u32 cable_power_class = 0, power_class_max = 0;
	u8 *cache = ppd->qsfp_info.cache;
	int ret = 0;

	ret = get_platform_config_field(ppd->dd, PLATFORM_CONFIG_SYSTEM_TABLE,
					0, SYSTEM_TABLE_QSFP_POWER_CLASS_MAX,
					&power_class_max, 4);
	if (ret)
		return ret;

	cable_power_class = hfi_get_qsfp_power_class(cache[QSFP_MOD_PWR_OFFS]);

	if (cable_power_class > power_class_max) {
		ppd->offline_disabled_reason =
			HFI_ODR_MASK(OPA_LINKDOWN_REASON_POWER_POLICY);

		dd_dev_err(ppd->dd,
			    "%s: Port disabled due to system power restrictions\n",
			    __func__);
		ret = -EPERM;
	}
	return ret;
}

/*
 * Return a special SerDes setting for low power AOC cables.  The power class
 * threshold and setting being used were all found by empirical testing.
 *
 * Summary of the logic:
 *
 * if (QSFP and (QSFP_TYPE == AOC || CU_WITH_LIMITING_ACTIVE_EQ)
 *	and QSFP_POWER_CLASS < 4)
 *     return 0xe
 * return 0; // leave at default
 */
static u8 aoc_low_power_setting(struct hfi_pportdata *ppd)
{
#if 0
/*
 * FXRTODO: Check if this is still needed
 */
	u8 *cache = ppd->qsfp_info.cache;
	int power_class;

	/* QSFP only */
	if (ppd->port_type != PORT_TYPE_QSFP)
		return 0; /* leave at default */

	/* active optical cables only */
	switch ((cache[QSFP_MOD_TECH_OFFS] & 0xF0) >> 4) {
	case 0x0 ... 0x9: /* fallthrough */
	case 0xC: /* fallthrough */
	case 0xE:
		/* active AOC */
		power_class =
			hfi_get_qsfp_power_class(cache[QSFP_MOD_PWR_OFFS]);
		if (power_class < QSFP_POWER_CLASS_4)
			return 0xe;
	}
#endif
	return 0; /* leave at default */
}

static void apply_tx_lanes(struct hfi_pportdata *ppd, u8 field_id,
			   u32 config_data, const char *message)
{
	u8 i;
	int ret = HCMD_SUCCESS;

	for (i = 0; i < 4; i++) {
		ret = load_8051_config(ppd, field_id, i, config_data);
		if (ret != HCMD_SUCCESS)
			dd_dev_err(ppd->dd, "%s: %s for lane %u failed\n",
				   message, __func__, i);
	}
}

static void apply_tunings(struct hfi_pportdata *ppd, u32 tx_preset_index,
			  u8 tuning_method, u32 total_atten,
			  u8 limiting_active)
{
	int ret = 0;
	u32 config_data = 0, tx_preset = 0;
	u8 precur = 0, attn = 0, postcur = 0, external_device_config = 0;
	u8 *cache = ppd->qsfp_info.cache;

	/* Pass tuning method to 8051 */
	hfi2_read_8051_config(ppd, LINK_TUNING_PARAMETERS, GENERAL_CONFIG,
			      &config_data);
	config_data &= ~(0xff << TUNING_METHOD_SHIFT);
	config_data |= ((u32)tuning_method << TUNING_METHOD_SHIFT);
	ret = load_8051_config(ppd, LINK_TUNING_PARAMETERS, GENERAL_CONFIG,
			       config_data);

	if (ret != HCMD_SUCCESS)
		dd_dev_err(ppd->dd, "%s: %d: Failed to set tuning method\n",
			   __func__, __LINE__);

	/* Set same channel loss for both TX and RX */
	config_data = 0 | (total_atten << 16) | (total_atten << 24);
	apply_tx_lanes(ppd, CHANNEL_LOSS_SETTINGS, config_data,
		       "Setting channel loss");

	/* Inform 8051 of cable capabilities */
	if (ppd->qsfp_info.cache_valid) {
		external_device_config =
			((cache[QSFP_MOD_PWR_OFFS] & 0x4) << 3) |
			((cache[QSFP_MOD_PWR_OFFS] & 0x8) << 2) |
			((cache[QSFP_EQ_INFO_OFFS] & 0x2) << 1) |
			(cache[QSFP_EQ_INFO_OFFS] & 0x4);
		ret = hfi2_read_8051_config(ppd, DC_HOST_COMM_SETTINGS,
					    GENERAL_CONFIG, &config_data);
		/* Clear, then set the external device config field */
		config_data &= ~(u32)0xFF;
		config_data |= external_device_config;
		ret = load_8051_config(ppd, DC_HOST_COMM_SETTINGS,
				       GENERAL_CONFIG, config_data);

		if (ret != HCMD_SUCCESS)
			dd_dev_err(ppd->dd,
				    "%s: %d: Failed set ext device config param\n",
				    __func__, __LINE__);
	}

	if (tx_preset_index == OPA_INVALID_INDEX) {
		if (ppd->port_type == PORT_TYPE_QSFP && limiting_active)
			dd_dev_err(ppd->dd,
				    "%s: %d: Invalid Tx preset index\n",
				    __func__, __LINE__);
		return;
	}

	/* Following for limiting active channels only */
	if (ppd->qsfp_info.limiting_active) {
		get_platform_config_field(ppd->dd,
					  PLATFORM_CONFIG_TX_PRESET_TABLE,
					  tx_preset_index,
					  TX_PRESET_TABLE_PRECUR,
					  &tx_preset, 4);
		precur = tx_preset;

		get_platform_config_field(ppd->dd,
					  PLATFORM_CONFIG_TX_PRESET_TABLE,
					  tx_preset_index,
					  TX_PRESET_TABLE_ATTN, &tx_preset, 4);
		attn = tx_preset;

		get_platform_config_field(ppd->dd,
					  PLATFORM_CONFIG_TX_PRESET_TABLE,
					  tx_preset_index,
					  TX_PRESET_TABLE_POSTCUR,
					  &tx_preset, 4);
		postcur = tx_preset;

		/*
		 * NOTES:
		 * o The aoc_low_power_setting is applied to all lanes even
		 *   though only lane 0's value is examined by the firmware.
		 * o A lingering low power setting after a cable swap does not
		 *   occur. On cable unplug, the 8051 is reset and restarted on
		 *   cable insert. This resets all settings to their default,
		 *   erasing any previous low power setting.
		 */
		config_data = precur | (attn << 8) | (postcur << 16) |
			      (aoc_low_power_setting(ppd) << 24);

		apply_tx_lanes(ppd, TX_EQ_SETTINGS, config_data,
			       "Applying TX settings");
	}
}

static int hfi_configure_qsfp(struct hfi_pportdata *ppd)
{
	int ret = 0;

	ppd->qsfp_info.limiting_active = 1;

	ret = set_qsfp_tx(ppd, 0);
	if (ret)
		return ret;

	ret = qual_power(ppd);
	if (ret)
		return ret;

	ret = qual_bitrate(ppd);
	if (ret)
		return ret;

	/*
	 * We'll change the QSFP memory contents from here on out, thus we set
	 * a flag here to remind ourselves to reset the QSFP module. This
	 * prevents reuse of stale settings established in our previous pass
	 * through.
	 */
	if (ppd->qsfp_info.reset_needed) {
		ret = hfi_reset_qsfp(ppd);
		if (ret)
			return ret;
		hfi_refresh_qsfp_cache(ppd, &ppd->qsfp_info);
	} else {
		ppd->qsfp_info.reset_needed = 1;
	}

	return hfi_set_qsfp_high_power(ppd);
}

static int tune_active_qsfp(struct hfi_pportdata *ppd, u32 *ptr_tx_preset,
			    u32 *ptr_rx_preset, u32 *ptr_total_atten)
{
	int ret;
	u16 lss = ppd->link_speed_supported, lse = ppd->link_speed_enabled;
	u8 *cache = ppd->qsfp_info.cache;

	ret = hfi_configure_qsfp(ppd);
	if (ret)
		return ret;

	if (cache[QSFP_EQ_INFO_OFFS] & 0x4) {
		ret = get_platform_config_field(
			ppd->dd,
			PLATFORM_CONFIG_PORT_TABLE, 0,
			PORT_TABLE_TX_PRESET_IDX_ACTIVE_EQ,
			ptr_tx_preset, 4);
		if (ret) {
			*ptr_tx_preset = OPA_INVALID_INDEX;
			return ret;
		}
	} else {
		ret = get_platform_config_field(
			ppd->dd,
			PLATFORM_CONFIG_PORT_TABLE, 0,
			PORT_TABLE_TX_PRESET_IDX_ACTIVE_NO_EQ,
			ptr_tx_preset, 4);
		if (ret) {
			*ptr_tx_preset = OPA_INVALID_INDEX;
			return ret;
		}
	}

	ret = get_platform_config_field(ppd->dd,
					PLATFORM_CONFIG_PORT_TABLE, 0,
					PORT_TABLE_RX_PRESET_IDX,
					ptr_rx_preset, 4);
	if (ret) {
		*ptr_rx_preset = OPA_INVALID_INDEX;
		return ret;
	}

	if ((lss & OPA_LINK_SPEED_25G) && (lse & OPA_LINK_SPEED_25G))
		get_platform_config_field(ppd->dd, PLATFORM_CONFIG_PORT_TABLE,
					  0, PORT_TABLE_LOCAL_ATTEN_25G,
					  ptr_total_atten, 4);

	apply_cdr_settings(ppd, *ptr_rx_preset, *ptr_tx_preset);

	apply_eq_settings(ppd, *ptr_rx_preset, *ptr_tx_preset);

	apply_rx_amplitude_settings(ppd, *ptr_rx_preset, *ptr_tx_preset);

	ret = set_qsfp_tx(ppd, 1);

	return ret;
}

static int tune_qsfp(struct hfi_pportdata *ppd,
		     u32 *ptr_tx_preset, u32 *ptr_rx_preset,
		     u8 *ptr_tuning_method, u32 *ptr_total_atten)
{
	u32 cable_atten = 0, remote_atten = 0, platform_atten = 0;
	u16 lss = ppd->link_speed_supported, lse = ppd->link_speed_enabled;
	int ret = 0;
	u8 *cache = ppd->qsfp_info.cache;

	switch ((cache[QSFP_MOD_TECH_OFFS] & 0xF0) >> 4) {
	case 0xA ... 0xB:
		ret = get_platform_config_field(
			ppd->dd,
			PLATFORM_CONFIG_PORT_TABLE, 0,
			PORT_TABLE_LOCAL_ATTEN_25G,
			&platform_atten, 4);
		if (ret)
			return ret;

		if ((lss & OPA_LINK_SPEED_25G) && (lse & OPA_LINK_SPEED_25G))
			cable_atten = cache[QSFP_CU_ATTEN_12G_OFFS];

		/* Fallback to configured attenuation if cable memory is bad */
		/*
		 * FXRTODO: Check if the condition cable_atten > 36
		 * still applies. The value will depend on the range of
		 * attenuation the FXR SerDes will support.
		 */
		if (cable_atten == 0 || cable_atten > 36) {
			ret = get_platform_config_field(
				ppd->dd,
				PLATFORM_CONFIG_SYSTEM_TABLE, 0,
				SYSTEM_TABLE_QSFP_ATTENUATION_DEFAULT_25G,
				&cable_atten, 4);
			if (ret)
				return ret;
		}

		ret = get_platform_config_field(ppd->dd,
						PLATFORM_CONFIG_PORT_TABLE, 0,
						PORT_TABLE_REMOTE_ATTEN_25G,
						&remote_atten, 4);
		if (ret)
			return ret;

		*ptr_total_atten = platform_atten + cable_atten + remote_atten;

		*ptr_tuning_method = OPA_PASSIVE_TUNING;
		break;
	case 0x0 ... 0x9: /* fallthrough */
	case 0xC: /* fallthrough */
	case 0xE:
		ret = tune_active_qsfp(ppd, ptr_tx_preset, ptr_rx_preset,
				       ptr_total_atten);
		if (ret)
			return ret;

		*ptr_tuning_method = OPA_ACTIVE_TUNING;
		break;
	case 0xD: /* fallthrough */
	case 0xF:
	default:
		dd_dev_warn(ppd->dd, "%s: Unknown/unsupported cable\n",
			    __func__);
		break;
	}
	return ret;
}

void tune_serdes(struct hfi_pportdata *ppd)
{
	int ret = 0;
	u32 total_atten = 0;
	u32 remote_atten = 0, platform_atten = 0;
	u32 rx_preset_index = OPA_INVALID_INDEX;
	u32 tx_preset_index = OPA_INVALID_INDEX;
	u8 tuning_method = 0, limiting_active = 0;
	struct hfi_devdata *dd = ppd->dd;

	/* The driver link ready state defaults to not ready */
	ppd->driver_link_ready = 0;
	ppd->offline_disabled_reason = HFI_ODR_MASK(OPA_LINKDOWN_REASON_NONE);

	/* The link defaults to enabled */
	ppd->link_enabled = 1;

	if (hfi_qsfp_mod_present(ppd))
		hfi_refresh_qsfp_cache(ppd, &ppd->qsfp_info);

	/*
	 * FXRTODO: Skip until silicon is available
	 */
	ppd->driver_link_ready = 1;
	return;

	switch (ppd->port_type) {
	case PORT_TYPE_DISCONNECTED:
		ppd->offline_disabled_reason =
			HFI_ODR_MASK(OPA_LINKDOWN_REASON_DISCONNECTED);
		dd_dev_warn(dd, "%s: %d: Port disconnected, disabling port\n",
			    __func__, __LINE__);
		goto bail;
	case PORT_TYPE_FIXED:
		/*
		 * platform atten, remote_atten pre-zeroed to catch
		 * error
		 */
		get_platform_config_field(ppd->dd, PLATFORM_CONFIG_PORT_TABLE,
					  0, PORT_TABLE_LOCAL_ATTEN_25G,
					  &platform_atten, 4);

		get_platform_config_field(ppd->dd, PLATFORM_CONFIG_PORT_TABLE,
					  0, PORT_TABLE_REMOTE_ATTEN_25G,
					  &remote_atten, 4);

		total_atten = platform_atten + remote_atten;

		tuning_method = OPA_PASSIVE_TUNING;
		break;
	case PORT_TYPE_VARIABLE:
		if (hfi_qsfp_mod_present(ppd)) {
			/*
			 * platform_atten, remote_atten pre-zeroed to
			 * catch error
			 */
			get_platform_config_field(ppd->dd,
						  PLATFORM_CONFIG_PORT_TABLE, 0,
						  PORT_TABLE_LOCAL_ATTEN_25G,
						  &platform_atten, 4);

			get_platform_config_field(ppd->dd,
						  PLATFORM_CONFIG_PORT_TABLE, 0,
						  PORT_TABLE_REMOTE_ATTEN_25G,
						  &remote_atten, 4);

			total_atten = platform_atten + remote_atten;

			tuning_method = OPA_PASSIVE_TUNING;
		} else {
			ppd->offline_disabled_reason =
			     HFI_ODR_MASK(OPA_LINKDOWN_REASON_CHASSIS_CONFIG);
			goto bail;
		}
		break;
	case PORT_TYPE_QSFP:
		if (hfi_qsfp_mod_present(ppd)) {
			hfi_refresh_qsfp_cache(ppd, &ppd->qsfp_info);

			if (ppd->qsfp_info.cache_valid) {
				ret = tune_qsfp(ppd,
						&tx_preset_index,
						&rx_preset_index,
						&tuning_method,
						&total_atten);

				/*
				 * We may have modified the QSFP memory,
				 * so update the cache to reflect the
				 * changes
				 */
				hfi_refresh_qsfp_cache(ppd, &ppd->qsfp_info);
				limiting_active =
					ppd->qsfp_info.limiting_active;
			} else {
				dd_dev_err(dd,
					   "%s: %d: Reading QSFP memory failed\n",
					   __func__, __LINE__);
				ret = -EINVAL;
			}

			if (ret)
				goto bail;
		} else {
			ppd->offline_disabled_reason =
			    HFI_ODR_MASK(
				OPA_LINKDOWN_REASON_LOCAL_MEDIA_NOT_INSTALLED);
			goto bail;
		}
		break;
	default:
		dd_dev_warn(ppd->dd, "%s: %d: Unknown port type\n",
			    __func__, __LINE__);
		ppd->port_type = PORT_TYPE_UNKNOWN;
		tuning_method = OPA_UNKNOWN_TUNING;
		total_atten = 0;
		limiting_active = 0;
		tx_preset_index = OPA_INVALID_INDEX;
		break;
	}

	if (ppd->offline_disabled_reason ==
			HFI_ODR_MASK(OPA_LINKDOWN_REASON_NONE))
		apply_tunings(ppd, tx_preset_index, tuning_method,
			      total_atten, limiting_active);

	if (!ret)
		ppd->driver_link_ready = 1;

	return;
bail:
	ppd->driver_link_ready = 0;
}

static int get_platform_config_field_metadata(struct hfi_devdata *dd,
					      int table, int field,
					      u32 *field_len_bits,
					      u32 *field_start_bits)
{
	struct platform_config_cache *pcfgcache = &dd->pcfg_cache;
	u32 *src_ptr = NULL;

	if (!pcfgcache->cache_valid)
		return -EINVAL;

	switch (table) {
	case PLATFORM_CONFIG_SYSTEM_TABLE:
		/* fall through */
	case PLATFORM_CONFIG_PORT_TABLE:
		/* fall through */
	case PLATFORM_CONFIG_RX_PRESET_TABLE:
		/* fall through */
	case PLATFORM_CONFIG_TX_PRESET_TABLE:
		/* fall through */
	case PLATFORM_CONFIG_QSFP_ATTEN_TABLE:
		/* fall through */
	case PLATFORM_CONFIG_VARIABLE_SETTINGS_TABLE:
		if (field && field < platform_config_table_limits[table])
			src_ptr =
			pcfgcache->config_tables[table].table_metadata + field;
		break;
	default:
		dd_dev_info(dd, "%s: Unknown table\n", __func__);
		break;
	}

	if (!src_ptr)
		return -EINVAL;

	if (field_start_bits)
		*field_start_bits = *src_ptr &
		      ((1 << METADATA_TABLE_FIELD_START_LEN_BITS) - 1);

	if (field_len_bits)
		*field_len_bits = (*src_ptr >> METADATA_TABLE_FIELD_LEN_SHIFT)
		       & ((1 << METADATA_TABLE_FIELD_LEN_LEN_BITS) - 1);

	return 0;
}

static void
get_integrated_platform_config_field(struct hfi_devdata *dd,
				     enum platform_config_table_type_encoding
				     table_type, int field_index, u32 *data)
{
	struct hfi_pportdata *ppd = dd->pport;
	u8 *cache = ppd->qsfp_info.cache;
	u32 tx_preset = 0;

	switch (table_type) {
	case PLATFORM_CONFIG_SYSTEM_TABLE:
		if (field_index == SYSTEM_TABLE_QSFP_POWER_CLASS_MAX)
			*data = ppd->max_power_class;
		else if (field_index ==
				SYSTEM_TABLE_QSFP_ATTENUATION_DEFAULT_25G)
			*data = ppd->default_atten;
		break;
	case PLATFORM_CONFIG_PORT_TABLE:
		if (field_index == PORT_TABLE_PORT_TYPE)
			*data = ppd->port_type;
		else if (field_index == PORT_TABLE_LOCAL_ATTEN_25G)
			*data = ppd->local_atten;
		else if (field_index == PORT_TABLE_REMOTE_ATTEN_25G)
			*data = ppd->remote_atten;
		break;
	case PLATFORM_CONFIG_RX_PRESET_TABLE:
		if (field_index == RX_PRESET_TABLE_QSFP_RX_CDR_APPLY)
			*data = (ppd->rx_preset & QSFP_RX_CDR_APPLY_SMASK) >>
				QSFP_RX_CDR_APPLY_SHIFT;
		else if (field_index == RX_PRESET_TABLE_QSFP_RX_EMP_APPLY)
			*data = (ppd->rx_preset & QSFP_RX_EMP_APPLY_SMASK) >>
				QSFP_RX_EMP_APPLY_SHIFT;
		else if (field_index == RX_PRESET_TABLE_QSFP_RX_AMP_APPLY)
			*data = (ppd->rx_preset & QSFP_RX_AMP_APPLY_SMASK) >>
				QSFP_RX_AMP_APPLY_SHIFT;
		else if (field_index == RX_PRESET_TABLE_QSFP_RX_CDR)
			*data = (ppd->rx_preset & QSFP_RX_CDR_SMASK) >>
				QSFP_RX_CDR_SHIFT;
		else if (field_index == RX_PRESET_TABLE_QSFP_RX_EMP)
			*data = (ppd->rx_preset & QSFP_RX_EMP_SMASK) >>
				QSFP_RX_EMP_SHIFT;
		else if (field_index == RX_PRESET_TABLE_QSFP_RX_AMP)
			*data = (ppd->rx_preset & QSFP_RX_AMP_SMASK) >>
				QSFP_RX_AMP_SHIFT;
		break;
	case PLATFORM_CONFIG_TX_PRESET_TABLE:
		if (cache[QSFP_EQ_INFO_OFFS] & 0x4)
			tx_preset = ppd->tx_preset_eq;
		else
			tx_preset = ppd->tx_preset_noeq;
		if (field_index == TX_PRESET_TABLE_PRECUR)
			*data = (tx_preset & TX_PRECUR_SMASK) >>
				TX_PRECUR_SHIFT;
		else if (field_index == TX_PRESET_TABLE_ATTN)
			*data = (tx_preset & TX_ATTN_SMASK) >>
				TX_ATTN_SHIFT;
		else if (field_index == TX_PRESET_TABLE_POSTCUR)
			*data = (tx_preset & TX_POSTCUR_SMASK) >>
				TX_POSTCUR_SHIFT;
		else if (field_index == TX_PRESET_TABLE_QSFP_TX_CDR_APPLY)
			*data = (tx_preset & QSFP_TX_CDR_APPLY_SMASK) >>
				QSFP_TX_CDR_APPLY_SHIFT;
		else if (field_index == TX_PRESET_TABLE_QSFP_TX_EQ_APPLY)
			*data = (tx_preset & QSFP_TX_EQ_APPLY_SMASK) >>
				QSFP_TX_EQ_APPLY_SHIFT;
		else if (field_index == TX_PRESET_TABLE_QSFP_TX_CDR)
			*data = (tx_preset & QSFP_TX_CDR_SMASK) >>
				QSFP_TX_CDR_SHIFT;
		else if (field_index == TX_PRESET_TABLE_QSFP_TX_EQ)
			*data = (tx_preset & QSFP_TX_EQ_SMASK) >>
				QSFP_TX_EQ_SHIFT;
		break;
	case PLATFORM_CONFIG_QSFP_ATTEN_TABLE:
	case PLATFORM_CONFIG_VARIABLE_SETTINGS_TABLE:
	default:
		break;
	}
}

/*
 * This is the central interface to getting data out of the platform config
 * file. It depends on hfi2_parse_platform_config() having populated the
 * platform_config_cache in hfi_devdata, and checks the cache_valid member to
 * validate the sanity of the cache.
 *
 * The non-obvious parameters:
 * @table_index: Acts as a look up key into which instance of the tables the
 * relevant field is fetched from.
 *
 * This applies to the data tables that have multiple instances. The port table
 * is an exception to this rule as each HFI only has one port and thus the
 * relevant table can be distinguished by hfi_id.
 *
 * @data: pointer to memory that will be populated with the field requested.
 * @len: length of memory pointed by @data in bytes.
 */
int get_platform_config_field(struct hfi_devdata *dd,
			      enum platform_config_table_type_encoding
			      table_type, int table_index, int field_index,
			      u32 *data, u32 len)
{
	int ret = 0, wlen = 0, seek = 0;
	u32 field_len_bits = 0, field_start_bits = 0, *src_ptr = NULL;
	struct platform_config_cache *pcfgcache = &dd->pcfg_cache;

	if (data)
		memset(data, 0, len);
	else
		return -EINVAL;

	if (dd->platform_config.from != FROM_LIB_FIRMWARE) {
		/*
		 * Use saved configuration from ppd for integrated platforms
		 */
		get_integrated_platform_config_field(dd, table_type,
						     field_index, data);
		return 0;
	}

	ret = get_platform_config_field_metadata(dd, table_type, field_index,
						 &field_len_bits,
						 &field_start_bits);
	if (ret)
		return ret;

	/* Convert length to bits */
	len *= 8;

	/* Our metadata function checked cache_valid and field_index for us */
	switch (table_type) {
	case PLATFORM_CONFIG_SYSTEM_TABLE:
		src_ptr = pcfgcache->config_tables[table_type].table;

		if (field_index != SYSTEM_TABLE_QSFP_POWER_CLASS_MAX) {
			if (len < field_len_bits)
				return -EINVAL;

			seek = field_start_bits / 8;
			wlen = field_len_bits / 8;

			src_ptr = (u32 *)((u8 *)src_ptr + seek);

			/*
			 * We expect the field to be byte aligned and whole
			 * byte lengths if we are here
			 */
			memcpy(data, src_ptr, wlen);
			return 0;
		}
		break;
	case PLATFORM_CONFIG_PORT_TABLE:
		src_ptr = pcfgcache->config_tables[table_type].table;
		break;
	case PLATFORM_CONFIG_RX_PRESET_TABLE:
		/* fall through */
	case PLATFORM_CONFIG_TX_PRESET_TABLE:
		/* fall through */
	case PLATFORM_CONFIG_QSFP_ATTEN_TABLE:
		/* fall through */
	case PLATFORM_CONFIG_VARIABLE_SETTINGS_TABLE:
		src_ptr = pcfgcache->config_tables[table_type].table;

		if (table_index <
			pcfgcache->config_tables[table_type].num_table)
			src_ptr += table_index;
		else
			src_ptr = NULL;
		break;
	default:
		dd_dev_info(dd, "%s: Unknown table\n", __func__);
		break;
	}

	if (!src_ptr || len < field_len_bits)
		return -EINVAL;

	src_ptr += (field_start_bits / 32);
	*data = (*src_ptr >> (field_start_bits % 32)) &
		((1 << field_len_bits) - 1);

	return 0;
}

static void get_port_type(struct hfi_pportdata *ppd)
{
	int ret;
	u32 temp;

	ret = get_platform_config_field(ppd->dd, PLATFORM_CONFIG_PORT_TABLE, 0,
					PORT_TABLE_PORT_TYPE, &temp,
					4);
	if (ret) {
		ppd->port_type = PORT_TYPE_UNKNOWN;
		return;
	}
	ppd->port_type = temp;
}

int hfi_bringup_serdes(struct hfi_pportdata *ppd)
{
	/* Set linkinit_reason on power up per OPA spec */
	ppd->linkinit_reason = OPA_LINKINIT_REASON_LINKUP;

	/*
	 * FXRTODO: Revisit later
	 */
#if 0
	/* one-time init of the LCB */
	init_lcb(dd);

	if (loopback) {
		ret = init_loopback(dd);
		if (ret < 0)
			return ret;
	}
#endif
	get_port_type(ppd);
	if (ppd->port_type == PORT_TYPE_QSFP) {
		hfi_set_qsfp_int_n(ppd, 0);
		hfi_wait_for_qsfp_init(ppd);
		hfi_set_qsfp_int_n(ppd, 1);
	}

	try_start_link(ppd);
	return 0;
}

void hfi_quiet_serdes(struct hfi_pportdata *ppd)
{
	/*
	 * Shut down the link and keep it down.   First turn off that the
	 * driver wants to allow the link to be up (driver_link_ready).
	 * Then make sure the link is not automatically restarted
	 * (link_enabled).  Cancel any pending restart.  And finally
	 * go offline.
	 */
	ppd->driver_link_ready = 0;
	ppd->link_enabled = 0;

	ppd->qsfp_retry_count = MAX_QSFP_RETRIES; /* prevent more retries */
	flush_delayed_work(&ppd->start_link_work);
	cancel_delayed_work_sync(&ppd->start_link_work);

	ppd->offline_disabled_reason =
			HFI_ODR_MASK(OPA_LINKDOWN_REASON_REBOOT);
	set_link_down_reason(ppd, OPA_LINKDOWN_REASON_REBOOT, 0,
			     OPA_LINKDOWN_REASON_REBOOT);
	hfi_set_link_state(ppd, HLS_DN_OFFLINE);
}
