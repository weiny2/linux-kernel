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

#include "hfi.h"
#include "efivar.h"

void get_platform_config(struct hfi1_devdata *dd)
{
	int ret = 0;
	unsigned long size = 0;
	u8 *temp_platform_config = NULL;

	ret = read_hfi1_efi_var(dd, "configuration", &size,
				(void **)&temp_platform_config);
	if (ret) {
		dd_dev_info(dd, "%s: Failed to get platform config var\n",
			    __func__);
		/* fall back to request firmware */
		platform_config_load = 1;
		goto bail;
	}

	dd->platform_config.data = temp_platform_config;
	dd->platform_config.size = size;

bail:
	/* exit */;
}

void free_platform_config(struct hfi1_devdata *dd)
{
	if (!platform_config_load) {
		/*
		 * was loaded from EFI, release memory
		 * allocated by read_efi_var
		 */
		kfree(dd->platform_config.data);
	}
	/*
	 * else do nothing, dispose_firmware will release
	 * struct firmware platform_config on driver exit
	 */
}

int set_qsfp_tx(struct hfi1_pportdata *ppd, int on)
{
	u8 tx_ctrl_byte = on ? 0x0 : 0xF;
	int ret = 0;

	ret = qsfp_write(ppd, ppd->dd->hfi1_id, QSFP_TX_CTRL_BYTE_OFFS,
			 &tx_ctrl_byte, 1);
	if (ret == 1)
		return 0;

	return ret;
}

static int qual_power(struct hfi1_pportdata *ppd)
{
	u32 cable_power_class = 0, power_class_max = 0;
	u8 *cache = ppd->qsfp_info.cache;
	int ret = 0;

	ret = get_platform_config_field(
		ppd->dd, PLATFORM_CONFIG_SYSTEM_TABLE, 0,
		SYSTEM_TABLE_QSFP_POWER_CLASS_MAX, &power_class_max, 4);

	cable_power_class = get_qsfp_power_class(cache[QSFP_MOD_PWR_OFFS]);

	if (cable_power_class > power_class_max)
		ppd->offline_disabled_reason =
			HFI1_ODR_MASK(OPA_LINKDOWN_REASON_POWER_POLICY);

	if (ppd->offline_disabled_reason ==
			HFI1_ODR_MASK(OPA_LINKDOWN_REASON_POWER_POLICY)) {
		dd_dev_info(
			ppd->dd,
			"%s: Port disabled due to system power restrictions\n",
			__func__);
		ret = -EPERM;
	}
	return ret;
}

static int qual_bitrate(struct hfi1_pportdata *ppd)
{
	u16 lss = ppd->link_speed_supported, lse = ppd->link_speed_enabled;
	u8 *cache = ppd->qsfp_info.cache;

	if ((lss & OPA_LINK_SPEED_25G) && (lse & OPA_LINK_SPEED_25G) &&
	    cache[QSFP_NOM_BIT_RATE_250_OFFS] < 0x64)
		ppd->offline_disabled_reason =
		HFI1_ODR_MASK(OPA_LINKDOWN_REASON_LINKSPEED_POLICY);

	if ((lss & OPA_LINK_SPEED_12_5G) && (lse & OPA_LINK_SPEED_12_5G) &&
	    cache[QSFP_NOM_BIT_RATE_100_OFFS] < 0x7D)
		ppd->offline_disabled_reason =
		HFI1_ODR_MASK(OPA_LINKDOWN_REASON_LINKSPEED_POLICY);

	if (ppd->offline_disabled_reason ==
			HFI1_ODR_MASK(OPA_LINKDOWN_REASON_LINKSPEED_POLICY)) {
		dd_dev_info(
			ppd->dd,
			"%s: Cable failed bitrate check, disabling port\n",
			__func__);
		return -EPERM;
	}
	return 0;
}

static int set_qsfp_high_power(struct hfi1_pportdata *ppd)
{
	u8 cable_power_class = 0, power_ctrl_byte = 0;
	u8 *cache = ppd->qsfp_info.cache;

	cable_power_class = get_qsfp_power_class(cache[QSFP_MOD_PWR_OFFS]);

	if (cable_power_class > QSFP_POWER_CLASS_1) {
		power_ctrl_byte = cache[QSFP_PWR_CTRL_BYTE_OFFS];

		power_ctrl_byte |= 1;
		power_ctrl_byte &= ~(0x2);

		qsfp_write(ppd, ppd->dd->hfi1_id, QSFP_PWR_CTRL_BYTE_OFFS,
			   &power_ctrl_byte, 1);

		if (cable_power_class > QSFP_POWER_CLASS_4) {
			power_ctrl_byte |= (1 << 2);
			qsfp_write(ppd, ppd->dd->hfi1_id,
				   QSFP_PWR_CTRL_BYTE_OFFS,
				   &power_ctrl_byte, 1);
		}

		/* SFF 8679 rev 1.7 LPMode Deassert time */
		msleep(300);
	}
	return 0;
}

static void apply_cdr_settings(
		struct hfi1_pportdata *ppd, u32 rx_preset_index,
		u32 tx_preset_index)
{
	u8 *cache = ppd->qsfp_info.cache;
	u8 cdr_ctrl_byte = cache[QSFP_CDR_CTRL_BYTE_OFFS];
	u32 tx_preset = 0, rx_preset = 0;
	int cable_power_class;

	cable_power_class = get_qsfp_power_class(cache[QSFP_MOD_PWR_OFFS]);

	if ((cache[QSFP_MOD_PWR_OFFS] & 0x4) &&
	    (cache[QSFP_CDR_INFO_OFFS] & 0x40)) {
		/* RX CDR present, bypass supported */
		if (cable_power_class <= QSFP_POWER_CLASS_3) {
			/* Power class <= 3, ignore config & turn RX CDR on */
			cdr_ctrl_byte |= 0xF;
			goto tx_cdr_control;
		}

		get_platform_config_field(
			ppd->dd, PLATFORM_CONFIG_RX_PRESET_TABLE,
			rx_preset_index, RX_PRESET_TABLE_QSFP_RX_CDR_APPLY,
			&rx_preset, 4);

		if (rx_preset) {
			get_platform_config_field(
				ppd->dd, PLATFORM_CONFIG_RX_PRESET_TABLE,
				rx_preset_index, RX_PRESET_TABLE_QSFP_RX_CDR,
				&rx_preset, 4);

			/* Expand cdr setting to all 4 lanes */
			rx_preset = (rx_preset | (rx_preset << 1) |
					(rx_preset << 2) | (rx_preset << 3));

			if (rx_preset) {
				cdr_ctrl_byte |= rx_preset;
			} else {
				cdr_ctrl_byte &= rx_preset;
				/* Preserve current TX CDR status */
				cdr_ctrl_byte |=
				 (cache[QSFP_CDR_CTRL_BYTE_OFFS]
					& 0xF0);
			}
		} else
			dd_dev_info(
				ppd->dd,
				"%s: RX_CDR_APPLY is set to disabled\n",
				__func__);
	}

tx_cdr_control:
	if ((cache[QSFP_MOD_PWR_OFFS] & 0x8) &&
	    (cache[QSFP_CDR_INFO_OFFS] & 0x80)) {
		/* TX CDR present, bypass supported */
		if (cable_power_class <= QSFP_POWER_CLASS_3) {
			/* Power class <= 3, ignore config & turn TX CDR on */
			cdr_ctrl_byte |= 0xF0;
			goto write_cdr_control;
		}

		get_platform_config_field(
			ppd->dd,
			PLATFORM_CONFIG_TX_PRESET_TABLE, tx_preset_index,
			TX_PRESET_TABLE_QSFP_TX_CDR_APPLY, &tx_preset, 4);

		if (tx_preset) {
			get_platform_config_field(
				ppd->dd,
				PLATFORM_CONFIG_TX_PRESET_TABLE,
				tx_preset_index,
				TX_PRESET_TABLE_QSFP_TX_CDR, &tx_preset, 4);

			/* Expand cdr setting to all 4 lanes */
			tx_preset = (tx_preset | (tx_preset << 1) |
					(tx_preset << 2) | (tx_preset << 3));

			if (tx_preset)
				cdr_ctrl_byte |= (tx_preset << 4);
			else
				/* Preserve current/determined RX CDR status */
				cdr_ctrl_byte &= ((tx_preset << 4) | 0xF);
		} else
			dd_dev_info(
				ppd->dd,
				"%s: TX_CDR_APPLY is set to disabled\n",
				__func__);
	}

write_cdr_control:
	qsfp_write(ppd, ppd->dd->hfi1_id, QSFP_CDR_CTRL_BYTE_OFFS,
		   &cdr_ctrl_byte, 1);
}

static void apply_eq_settings(struct hfi1_pportdata *ppd,
			      u32 rx_preset_index, u32 tx_preset_index)
{
	u32 tx_preset = 0, rx_preset = 0;
	u8 tx_eq = 0, rx_eq = 0, *cache = ppd->qsfp_info.cache;
	int ret = 0;

	/* Disable adaptive TX EQ if present */
	if (cache[QSFP_EQ_INFO_OFFS] & 0x8) {
		tx_eq = cache[(128 * 3) + 241];
		tx_eq &= 0xF0;
		qsfp_write(ppd, ppd->dd->hfi1_id, (256 * 3) + 241, &tx_eq, 1);
	}

	if (cache[QSFP_EQ_INFO_OFFS] & 0x4) {
		ret = get_platform_config_field(
			ppd->dd, PLATFORM_CONFIG_TX_PRESET_TABLE,
			tx_preset_index, TX_PRESET_TABLE_QSFP_TX_EQ_APPLY,
			&tx_preset, 4);

		if (tx_preset) {
			ret = get_platform_config_field(
				ppd->dd, PLATFORM_CONFIG_TX_PRESET_TABLE,
				tx_preset_index, TX_PRESET_TABLE_QSFP_TX_EQ,
				&tx_preset, 4);

			if (!(cache[2] & 4)) {
				/* Page 03 present */
				if (((cache[(128 * 3) + 224] & 0xF0) >> 4)
				    < tx_preset) {
					dd_dev_info(
						ppd->dd,
						"%s: TX EQ %x unsupported\n",
						__func__, tx_preset);

					dd_dev_info(
						ppd->dd,
						"%s: Applying EQ %x\n",
						__func__, cache[608] & 0xF0);

					tx_preset = (cache[608] & 0xF0) >> 4;
				}
				tx_eq = tx_preset | (tx_preset << 4);

				qsfp_write(ppd, ppd->dd->hfi1_id,
					   (256 * 3) + 234, &tx_eq, 1);
				qsfp_write(ppd, ppd->dd->hfi1_id,
					   (256 * 3) + 235, &tx_eq, 1);
			} else
				dd_dev_info(
					ppd->dd,
					"%s: Upper page 03 not present\n",
					__func__);
		} else
			dd_dev_info(
				ppd->dd,
				"%s: TX_EQ_APPLY is set to disabled\n",
				__func__);
	}

	if (cache[QSFP_EQ_INFO_OFFS] & 0x2) {
		ret = get_platform_config_field(
			ppd->dd, PLATFORM_CONFIG_RX_PRESET_TABLE,
			rx_preset_index, RX_PRESET_TABLE_QSFP_RX_EMP_APPLY,
			&rx_preset, 4);

		if (rx_preset) {
			ret = get_platform_config_field(
				ppd->dd, PLATFORM_CONFIG_RX_PRESET_TABLE,
				rx_preset_index, RX_PRESET_TABLE_QSFP_RX_EMP,
				&rx_preset, 4);

			if (!(cache[2] & 4)) {
				/* Page 03 present */
				if ((cache[(128 * 3) + 224] & 0xF)
				    < rx_preset) {
					dd_dev_info(
						ppd->dd,
						"%s: Requested RX EMP %x\n",
						__func__, rx_preset);

					dd_dev_info(
						ppd->dd,
						"%s: Applying supported EMP %x\n",
						__func__, cache[608] & 0xF);

					rx_preset = cache[608] & 0xF;
				}
				rx_eq = rx_preset | (rx_preset << 4);

				qsfp_write(ppd, ppd->dd->hfi1_id,
					   (256 * 3) + 236, &rx_eq, 1);
				qsfp_write(ppd, ppd->dd->hfi1_id,
					   (256 * 3) + 237, &rx_eq, 1);
			} else
				dd_dev_info(
					ppd->dd,
					"%s: Upper page 03 not present\n",
					__func__);

		} else
			dd_dev_info(
				ppd->dd,
				"%s: RX_EMP_APPLY is set to disabled\n",
				__func__);
	}
}

static void apply_rx_amplitude_settings(
		struct hfi1_pportdata *ppd, u32 rx_preset_index,
		u32 tx_preset_index)
{
	u32 rx_preset = 0;
	u8 rx_amp = 0, i = 0, preferred = 0, *cache = ppd->qsfp_info.cache;

	if (cache[QSFP_EQ_INFO_OFFS] & 0x1) {
		get_platform_config_field(
			ppd->dd, PLATFORM_CONFIG_RX_PRESET_TABLE,
			rx_preset_index, RX_PRESET_TABLE_QSFP_RX_AMP_APPLY,
			&rx_preset, 4);

		if (rx_preset) {
			get_platform_config_field(
				ppd->dd, PLATFORM_CONFIG_RX_PRESET_TABLE,
				rx_preset_index, RX_PRESET_TABLE_QSFP_RX_AMP,
				&rx_preset, 4);

			dd_dev_info(ppd->dd, "%s: Requested RX AMP %x\n",
				    __func__, rx_preset);

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
				dd_dev_info(
					ppd->dd,
					"No supported RX AMP, not applying\n");
				goto bail;
			}

			dd_dev_info(
				ppd->dd, "%s: Applying RX AMP %x\n",
				__func__, preferred);

			rx_amp = preferred | (preferred << 4);
			qsfp_write(ppd, ppd->dd->hfi1_id, (256 * 3) + 238,
				   &rx_amp, 1);
			qsfp_write(ppd, ppd->dd->hfi1_id, (256 * 3) + 239,
				   &rx_amp, 1);
		} else
			dd_dev_info(
				ppd->dd,
				"%s: RX_AMP_APPLY is set to disabled\n",
				__func__);
	}
bail:
	/* do nothing */;
}

#define OPA_INVALID_INDEX 0xFFF

static int apply_tunings(
		struct hfi1_pportdata *ppd, u32 tx_preset_index,
		u8 tuning_method, u32 total_atten, u8 limiting_active)
{
	int ret = 0;
	u32 config_data = 0, tx_preset = 0;
	u8 precur = 0, attn = 0, postcur = 0, external_device_config = 0;
	u8 *cache = ppd->qsfp_info.cache;

	/* Enable external device config if channel is limiting active */
	ret = read_8051_config(ppd->dd, LINK_OPTIMIZATION_SETTINGS,
			       GENERAL_CONFIG, &config_data);
	config_data |= limiting_active;
	ret = load_8051_config(ppd->dd, LINK_OPTIMIZATION_SETTINGS,
			       GENERAL_CONFIG, config_data);
	if (ret != HCMD_SUCCESS)
		dd_dev_info(
			ppd->dd,
			"%s: Failed to set enable external device config\n",
			__func__);

	/* Pass tuning method to 8051 */
	ret = read_8051_config(ppd->dd, LINK_TUNING_PARAMETERS, GENERAL_CONFIG,
			       &config_data);
	config_data |= tuning_method;
	ret = load_8051_config(ppd->dd, LINK_TUNING_PARAMETERS, GENERAL_CONFIG,
			       config_data);
	if (ret != HCMD_SUCCESS)
		dd_dev_info(ppd->dd, "%s: Failed to set tuning method\n",
			    __func__);

	/* Set same channel loss for both TX and RX */
	config_data = 0 | (total_atten << 16) | (total_atten << 24);
	ret = load_8051_config(ppd->dd, CHANNEL_LOSS_SETTINGS, 0, config_data);
	if (ret != HCMD_SUCCESS)
		dd_dev_info(
			ppd->dd,
			"%s: Failed to set channel loss for lane 0\n",
			__func__);

	ret = load_8051_config(ppd->dd, CHANNEL_LOSS_SETTINGS, 1, config_data);
	if (ret != HCMD_SUCCESS)
		dd_dev_info(
			ppd->dd,
			"%s: Failed to set channel loss for lane 1\n",
			__func__);

	ret = load_8051_config(ppd->dd, CHANNEL_LOSS_SETTINGS, 2, config_data);
	if (ret != HCMD_SUCCESS)
		dd_dev_info(
			ppd->dd,
			"%s: Failed to set channel loss for lane 2\n",
			__func__);

	ret = load_8051_config(ppd->dd, CHANNEL_LOSS_SETTINGS, 3, config_data);
	if (ret != HCMD_SUCCESS)
		dd_dev_info(
			ppd->dd,
			"%s: Failed to set channel loss for lane 3\n",
			__func__);

	/* Inform 8051 of cable capabilities */
	if (ppd->qsfp_info.cache_valid) {
		external_device_config =
			((cache[QSFP_MOD_PWR_OFFS] & 0x4) << 3) |
			((cache[QSFP_MOD_PWR_OFFS] & 0x8) << 2) |
			((cache[QSFP_EQ_INFO_OFFS] & 0x2) << 1) |
			(cache[QSFP_EQ_INFO_OFFS] & 0x4);
		ret = read_8051_config(ppd->dd, DC_HOST_COMM_SETTINGS,
				       GENERAL_CONFIG, &config_data);
		/* Clear, then set the external device config field */
		config_data &= ~(0xFF << 24);
		config_data |= (external_device_config << 24);
		ret = load_8051_config(ppd->dd, DC_HOST_COMM_SETTINGS,
				       GENERAL_CONFIG, config_data);
		if (ret != HCMD_SUCCESS)
			dd_dev_info(
				ppd->dd,
				"%s: Failed set ext device config params\n",
				__func__);
	}

	if (tx_preset_index == OPA_INVALID_INDEX) {
		if (ppd->port_type == PORT_TYPE_QSFP && limiting_active)
			dd_dev_info(ppd->dd, "%s: Invalid Tx preset index\n",
				    __func__);
		goto bail;
	}

	/* Following for limiting active channels only */
	ret = get_platform_config_field(
		ppd->dd, PLATFORM_CONFIG_TX_PRESET_TABLE, tx_preset_index,
		TX_PRESET_TABLE_PRECUR, &tx_preset, 4);
	precur = tx_preset;

	ret = get_platform_config_field(
		ppd->dd, PLATFORM_CONFIG_TX_PRESET_TABLE,
		tx_preset_index, TX_PRESET_TABLE_ATTN, &tx_preset, 4);
	attn = tx_preset;

	ret = get_platform_config_field(
		ppd->dd, PLATFORM_CONFIG_TX_PRESET_TABLE,
		tx_preset_index, TX_PRESET_TABLE_POSTCUR, &tx_preset, 4);
	postcur = tx_preset;

	config_data = precur | (attn << 8) | (postcur << 16);

	ret = load_8051_config(ppd->dd, TX_EQ_SETTINGS, 0, config_data);
	if (ret != HCMD_SUCCESS)
		dd_dev_info(
			ppd->dd,
			"%s: Applying TX settings to lane 0 failed\n",
			__func__);

	ret = load_8051_config(ppd->dd, TX_EQ_SETTINGS, 1, config_data);
	if (ret != HCMD_SUCCESS)
		dd_dev_info(
			ppd->dd,
			"%s: Applying TX settings to lane 1 failed\n",
			__func__);

	ret = load_8051_config(ppd->dd, TX_EQ_SETTINGS, 2, config_data);
	if (ret != HCMD_SUCCESS)
		dd_dev_info(
			ppd->dd,
			"%s: Applying TX settings to lane 2 failed\n",
			__func__);

	ret = load_8051_config(ppd->dd, TX_EQ_SETTINGS, 3, config_data);
	if (ret != HCMD_SUCCESS)
		dd_dev_info(
			ppd->dd,
			"%s: Applying TX settings to lane 3 failed\n",
			__func__);

bail:
	if (ret == HCMD_SUCCESS)
		ret = 0;

	return ret;
}

static int tune_active_qsfp(struct hfi1_pportdata *ppd, u32 *ptr_tx_preset,
			    u32 *ptr_rx_preset, u32 *ptr_total_atten)
{
	int ret;
	u16 lss = ppd->link_speed_supported, lse = ppd->link_speed_enabled;
	u8 *cache = ppd->qsfp_info.cache;

	ret = acquire_chip_resource(ppd->dd, qsfp_resource(ppd->dd), QSFP_WAIT);
	if (ret) {
		dd_dev_err(ppd->dd, "%s: hfi%d: cannot lock i2c chain\n",
			   __func__, (int)ppd->dd->hfi1_id);
		goto bail_no_unlock;
	}

	ppd->qsfp_info.limiting_active = 1;

	ret = set_qsfp_tx(ppd, 0);
	if (ret)
		goto bail;

	ret = qual_power(ppd);
	if (ret)
		goto bail;

	ret = qual_bitrate(ppd);
	if (ret)
		goto bail;

	if (ppd->qsfp_info.reset_needed) {
		reset_qsfp(ppd);
		ppd->qsfp_info.reset_needed = 0;
		refresh_qsfp_cache(ppd, &ppd->qsfp_info);
	} else {
		ppd->qsfp_info.reset_needed = 1;
	}

	set_qsfp_high_power(ppd);

	if (cache[QSFP_EQ_INFO_OFFS] & 0x4) {
		ret = get_platform_config_field(
			ppd->dd,
			PLATFORM_CONFIG_PORT_TABLE, 0,
			PORT_TABLE_TX_PRESET_IDX_ACTIVE_EQ,
			ptr_tx_preset, 4);
		if (ret) {
			*ptr_tx_preset = OPA_INVALID_INDEX;
			goto bail;
		}
	} else {
		ret = get_platform_config_field(
			ppd->dd,
			PLATFORM_CONFIG_PORT_TABLE, 0,
			PORT_TABLE_TX_PRESET_IDX_ACTIVE_NO_EQ,
			ptr_tx_preset, 4);
		if (ret) {
			*ptr_tx_preset = OPA_INVALID_INDEX;
			goto bail;
		}
	}

	ret = get_platform_config_field(
		ppd->dd, PLATFORM_CONFIG_PORT_TABLE, 0,
		PORT_TABLE_RX_PRESET_IDX, ptr_rx_preset, 4);
	if (ret) {
		*ptr_rx_preset = OPA_INVALID_INDEX;
		goto bail;
	}

	if ((lss & OPA_LINK_SPEED_25G) && (lse & OPA_LINK_SPEED_25G))
		ret = get_platform_config_field(
			ppd->dd, PLATFORM_CONFIG_PORT_TABLE, 0,
			PORT_TABLE_LOCAL_ATTEN_25G, ptr_total_atten, 4);
	else if ((lss & OPA_LINK_SPEED_12_5G) && (lse & OPA_LINK_SPEED_12_5G))
		ret = get_platform_config_field(
			ppd->dd, PLATFORM_CONFIG_PORT_TABLE, 0,
			PORT_TABLE_LOCAL_ATTEN_12G, ptr_total_atten, 4);

	apply_cdr_settings(ppd, *ptr_rx_preset, *ptr_tx_preset);

	apply_eq_settings(ppd, *ptr_rx_preset, *ptr_tx_preset);

	apply_rx_amplitude_settings(ppd, *ptr_rx_preset, *ptr_tx_preset);

	ret = set_qsfp_tx(ppd, 1);

bail:
	release_chip_resource(ppd->dd, qsfp_resource(ppd->dd));
bail_no_unlock:
	return ret;
}

static int tune_qsfp(struct hfi1_pportdata *ppd,
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

		if ((lss & OPA_LINK_SPEED_25G) && (lse & OPA_LINK_SPEED_25G))
			cable_atten = cache[QSFP_CU_ATTEN_12G_OFFS];

		if ((lss & OPA_LINK_SPEED_12_5G) &&
		    (lse & OPA_LINK_SPEED_12_5G))
			cable_atten = cache[QSFP_CU_ATTEN_7G_OFFS];

		/* Fallback to configured attenuation if cable memory is bad */
		if (cable_atten == 0 || cable_atten > 36)
			ret = get_platform_config_field(
				ppd->dd,
				PLATFORM_CONFIG_SYSTEM_TABLE, 0,
				SYSTEM_TABLE_QSFP_ATTENUATION_DEFAULT_25G,
				&cable_atten, 4);

		ret = get_platform_config_field(
			ppd->dd, PLATFORM_CONFIG_PORT_TABLE, 0,
			PORT_TABLE_REMOTE_ATTEN_25G, &remote_atten, 4);

		*ptr_total_atten = platform_atten + cable_atten + remote_atten;

		*ptr_tuning_method = 0;
		break;
	case 0x0 ... 0x9: /* fallthrough */
	case 0xC: /* fallthrough */
	case 0xE:
		ret = tune_active_qsfp(ppd, ptr_tx_preset, ptr_rx_preset,
				       ptr_total_atten);
		if (ret)
			goto bail;

		*ptr_tuning_method = 1;
		break;
	case 0xD: /* fallthrough */
	case 0xF:
	default:
		dd_dev_info(ppd->dd, "%s: Unknown/unsupported cable\n",
			    __func__);
		break;
	}
bail:
	return ret;
}

int tune_serdes(struct hfi1_pportdata *ppd)
{
	int ret = 0;
	u32 total_atten = 0;
	u32 cable_atten = 0, remote_atten = 0, platform_atten = 0;
	u32 rx_preset_index, tx_preset_index;
	u8 tuning_method = 0, limiting_active = 0;
	struct hfi1_devdata *dd = ppd->dd;

	rx_preset_index = OPA_INVALID_INDEX;
	tx_preset_index = OPA_INVALID_INDEX;

	/* the link defaults to enabled */
	ppd->link_enabled = 1;
	/* the driver defaults to not ready for the link to go up */
	ppd->driver_link_ready = 0;
	ppd->offline_disabled_reason = HFI1_ODR_MASK(OPA_LINKDOWN_REASON_NONE);

	/* Skip the tuning for testing and simulations */
	if (loopback || ppd->dd->icode == ICODE_FUNCTIONAL_SIMULATOR) {
		ppd->driver_link_ready = 1;
		return 0;
	}

	ret = get_platform_config_field(ppd->dd, PLATFORM_CONFIG_PORT_TABLE, 0,
					PORT_TABLE_PORT_TYPE, &ppd->port_type,
					4);
	if (ret)
		ppd->port_type = PORT_TYPE_UNKNOWN;

	switch (ppd->port_type) {
	case PORT_TYPE_DISCONNECTED:
		ppd->offline_disabled_reason =
			HFI1_ODR_MASK(OPA_LINKDOWN_REASON_DISCONNECTED);
		dd_dev_info(dd, "%s: Port disconnected, disabling port\n",
			    __func__);
		goto bail;
	case PORT_TYPE_FIXED:
		cable_atten = 0;
		ret = get_platform_config_field(
			ppd->dd, PLATFORM_CONFIG_PORT_TABLE, 0,
			PORT_TABLE_LOCAL_ATTEN_25G, &platform_atten, 4);

		ret = get_platform_config_field(
			ppd->dd, PLATFORM_CONFIG_PORT_TABLE, 0,
			PORT_TABLE_REMOTE_ATTEN_25G, &remote_atten, 4);

		total_atten = platform_atten + remote_atten;

		tuning_method = 0;
		break;
	case PORT_TYPE_VARIABLE:
		if (qsfp_mod_present(ppd)) {
			cable_atten = 0;
			ret = get_platform_config_field(
				ppd->dd, PLATFORM_CONFIG_PORT_TABLE, 0,
				PORT_TABLE_LOCAL_ATTEN_25G,
				&platform_atten, 4);

			ret = get_platform_config_field(
				ppd->dd, PLATFORM_CONFIG_PORT_TABLE, 0,
				PORT_TABLE_REMOTE_ATTEN_25G,
				&remote_atten, 4);

			total_atten = platform_atten + remote_atten;

			tuning_method = 0;
		} else
			ppd->offline_disabled_reason =
			     HFI1_ODR_MASK(OPA_LINKDOWN_REASON_CHASSIS_CONFIG);
		break;
	case PORT_TYPE_QSFP:
		if (qsfp_mod_present(ppd)) {
			refresh_qsfp_cache(ppd, &ppd->qsfp_info);

			if (ppd->qsfp_info.cache_valid) {
				ret = tune_qsfp(ppd, &tx_preset_index,
						&rx_preset_index,
						&tuning_method,
						&total_atten);

				limiting_active =
						ppd->qsfp_info.limiting_active;

				/*
				 * We may have modified the QSFP memory, so
				 * update the cache to reflect the changes
				 */
				refresh_qsfp_cache(ppd, &ppd->qsfp_info);
				if (ret)
					goto bail;
			} else {
				dd_dev_info(dd,
					    "%s: Reading QSFP memory failed\n",
						__func__);
				goto bail;
			}
		} else
			ppd->offline_disabled_reason =
			   HFI1_ODR_MASK(
				OPA_LINKDOWN_REASON_LOCAL_MEDIA_NOT_INSTALLED);
		break;
	default:
		dd_dev_info(ppd->dd, "%s: Unknown port type\n", __func__);
		ppd->port_type = PORT_TYPE_UNKNOWN;
		tuning_method = 2;
		total_atten = 0;
		limiting_active = 0;
		tx_preset_index = OPA_INVALID_INDEX;
		break;
	}

	if (ppd->offline_disabled_reason ==
			HFI1_ODR_MASK(OPA_LINKDOWN_REASON_NONE))
		ret = apply_tunings(ppd, tx_preset_index, tuning_method,
				    total_atten,
				    limiting_active);

	if (!ret)
		ppd->driver_link_ready = 1;

	return 0;
bail:
	ppd->driver_link_ready = 0;

	return -1;
}
