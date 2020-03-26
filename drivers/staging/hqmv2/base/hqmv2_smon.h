/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
 * Copyright(c) 2016-2018 Intel Corporation
 */

#ifndef __HQMV2_SMON_H
#define __HQMV2_SMON_H

#include "hqmv2_hw_types.h"
#include "hqmv2_osdep_types.h"

enum hqmv2_chp_smon_component {
	/* IDs 0-2 are unused */
	SMON_COMP_DQED = 3,
	SMON_COMP_QED,
	SMON_COMP_HIST_LIST,
};

enum hqmv2_chp_smon_hcw_type {
	SMON_HCW_NOOP,
	SMON_HCW_BAT_T,
	SMON_HCW_COMP,
	SMON_HCW_COMP_T,
	SMON_HCW_REL,
	/* IDs 5-7 are unused */
	SMON_HCW_ENQ = 8,
	SMON_HCW_ENQ_T,
	SMON_HCW_RENQ,
	SMON_HCW_RENQ_T,
	SMON_HCW_FRAG,
	SMON_HCW_FRAG_T,
};

void hqmv2_smon_setup_sys_iosf_measurements(struct hqmv2_hw *hw, u32 max);

void hqmv2_smon_setup_sys_dm_measurements(struct hqmv2_hw *hw, u32 max);

void hqmv2_smon_setup_chp_ing_egr_measurements(struct hqmv2_hw *hw, u32 max);

void hqmv2_smon_setup_chp_pp_count_measurement(struct hqmv2_hw *hw, u32 max);

void hqmv2_smon_setup_chp_avg_measurement(struct hqmv2_hw *hw,
					  enum hqmv2_chp_smon_component,
					  u32 max);

void hqmv2_smon_setup_chp_hcw_measurements(struct hqmv2_hw *hw,
					   enum hqmv2_chp_smon_hcw_type type_1,
					   enum hqmv2_chp_smon_hcw_type type_2,
					   u32 max);

enum hqmv2_smon_meas_type {
	SMON_MEASURE_CNT,
	SMON_MEASURE_AVG
};

void hqmv2_smon_read_sys_perf_counter(struct hqmv2_hw *hw,
				      enum hqmv2_smon_meas_type type,
				      u32 counter[2]);

void hqmv2_smon_read_chp_perf_counter(struct hqmv2_hw *hw,
				      enum hqmv2_smon_meas_type type,
				      u32 counter[2]);

#endif /* __HQMV2_SMON_H */
