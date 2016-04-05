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
#include "opa_hfi.h"

static char *platform_config_name = DEFAULT_PLATFORM_CONFIG_NAME;

void hfi2_free_platform_config(struct hfi_devdata *dd)
{
	switch (dd->platform_config.from) {
	case NOT_READ_YET:
		/* do nothing */
		break;
	case FROM_EFI:
		kfree(dd->platform_config.data);
		break;
	case FROM_LIB_FIRMWARE:
		release_firmware(dd->platform_config.firmware);
		break;
	default:
		dd_dev_warn(dd, "%s() %d : don't know where platform_config is read from: %d\n",
			__func__, __LINE__, dd->platform_config.from);
		break;
	}
}

/*
	read platform_config file
*/
void hfi2_get_platform_config(struct hfi_devdata *dd)
{
	int ret = 0;
	unsigned long size = 0;
	u8 *temp_platform_config = NULL;

	dd->platform_config.from = NOT_READ_YET;
	dd->platform_config.firmware = NULL;
	dd->platform_config.data = NULL;
	dd->platform_config.size = 0;
#if 1 /* FXRTODO: implement read_hfi2_efi_var. */
	ret = -EACCES;
#else
	ret = read_hfi1_efi_var(dd, "configuration", &size,
		(void **)&temp_platform_config);
#endif
	if (ret == 0) {
		dd_dev_info(dd, "%s() %d : get platform config var from EFI\n",
			__func__, __LINE__);
		dd->platform_config.from = FROM_EFI;
		dd->platform_config.data = temp_platform_config;
		dd->platform_config.size = size;
	} else {
		ret = request_firmware(&dd->platform_config.firmware,
			platform_config_name, &dd->pcidev->dev);
		if (ret == 0) {
			dd->platform_config.from = FROM_LIB_FIRMWARE;
			dd->platform_config.data = dd->platform_config.firmware->data;
			dd->platform_config.size = dd->platform_config.firmware->size;
		} else { /* can't read from both */
			dd_dev_warn(dd, "%s() %d : can't read: %s\n",
				__func__, __LINE__, platform_config_name);
		}
	}
}

/*
 * This function is a helper function for parse_platform_config(...) and
 * does not check for validity of the platform configuration cache
 * (because we know it is invalid as we are building up the cache).
 * As such, this should not be called from anywhere other than
 * parse_platform_config
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

	if (meta_ver < 5) {
		dd_dev_info(
			dd, "%s:Please update platform config\n", __func__);
		return -EINVAL;
	}
	return 0;
}

/*
	dd->platform_config -> dd->pcfg_cache
*/
int hfi2_parse_platform_config(struct hfi_devdata *dd)
{
	struct platform_config_cache *pcfgcache = &dd->pcfg_cache;
	u32 *ptr = NULL;
	u32 header1 = 0, header2 = 0, magic_num = 0, crc = 0, file_length = 0;
	u32 record_idx = 0, table_type = 0, table_length_dwords = 0;
	int ret = -EINVAL; /* assume failure */

	if (!dd->platform_config.data) {
		dd_dev_warn(dd, "%s() %d: Missing config file\n",
			__func__, __LINE__);
		goto bail;
	}
	ptr = (u32 *)dd->platform_config.data;

	magic_num = *ptr;
	ptr++;
	if (magic_num != PLATFORM_CONFIG_MAGIC_NUM) {
		dd_dev_warn(dd, "%s() %d: Bad config file: 0x%x expect 0x%x\n",
			__func__, __LINE__, magic_num, PLATFORM_CONFIG_MAGIC_NUM);
		goto bail;
	}

	/* Field is file size in DWORDs */
	file_length = (*ptr) * 4;
	ptr++;

	if (file_length > dd->platform_config.size) {
		dd_dev_info(dd, "%s:File claims to be larger than read size\n",
			    __func__);
		goto bail;
	} else if (file_length < dd->platform_config.size) {
		dd_dev_info(
		    dd,
		    "%s:File claims to be smaller than read size, continuing\n",
		    __func__);
	}
	/* exactly equal, perfection */

	/*
	 * In both cases where we proceed, using the self-reported file length
	 * is the safer option
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
				       (ptr - (u32 *)dd->platform_config.data));
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
					    "%s: Unknown meta table %d, offset %ld\n",
					    __func__, table_type,
				(ptr - (u32 *)dd->platform_config.data));
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
