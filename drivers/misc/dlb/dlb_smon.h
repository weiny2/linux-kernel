/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
 * Copyright(c) 2016-2020 Intel Corporation
 */

#ifndef __DLB_SMON_H
#define __DLB_SMON_H

#include <linux/types.h>

#include "dlb_hw_types.h"

enum dlb_chp_smon_component {
	/* IDs 0-2 are unused */
	SMON_COMP_DQED = 3,
	SMON_COMP_QED,
	SMON_COMP_HIST_LIST,
};

enum dlb_chp_smon_hcw_type {
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

void dlb_smon_setup_sys_iosf_measurements(struct dlb_hw *hw, u32 max);

void dlb_smon_setup_chp_ing_egr_measurements(struct dlb_hw *hw, u32 max);

void dlb_smon_setup_chp_pp_count_measurement(struct dlb_hw *hw, u32 max);

void dlb_smon_setup_chp_avg_measurement(struct dlb_hw *hw,
					enum dlb_chp_smon_component,
					u32 max);

void dlb_smon_setup_chp_hcw_measurements(struct dlb_hw *hw,
					 enum dlb_chp_smon_hcw_type type_1,
					 enum dlb_chp_smon_hcw_type type_2,
					 u32 max);

enum dlb_smon_meas_type {
	SMON_MEASURE_CNT,
	SMON_MEASURE_AVG
};

void dlb_smon_read_sys_perf_counter(struct dlb_hw *hw,
				    enum dlb_smon_meas_type type,
				    u32 counter[2]);

void dlb_smon_read_chp_perf_counter(struct dlb_hw *hw,
				    enum dlb_smon_meas_type type,
				    u32 counter[2]);

#endif /* __DLB_SMON_H */
