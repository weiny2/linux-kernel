// SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
/* Copyright(c) 2016-2018 Intel Corporation */

#include "hqmv2_hw_types.h"
#include "hqmv2_osdep.h"
#include "hqmv2_osdep_types.h"
#include "hqmv2_smon.h"
#include "hqmv2_regs.h"

struct hqmv2_smon_cfg {
	u8 mode[2];
	u8 cmp[2];
	u32 mask[2];
	u32 cnt[2];
	u32 timer;
	u32 max_timer;
	u8 fn[2];
	u8 fn_cmp[2];
	u8 smon_mode;
	u8 stop_counter_mode;
	u8 stop_timer_mode;
};

#define DEFAULT_SMON_CFG \
	{{0, 0}, {0, 0}, {0xFFFFFFFF, 0xFFFFFFFF}, \
	{0, 0}, 0, 0, {0, 0}, {0, 0}, 0, 0, 0}

static void hqmv2_smon_setup_sys_perf_smon(struct hqmv2_hw *hw,
					   struct hqmv2_smon_cfg *cfg)
{
	union hqmv2_sys_smon_cfg0 r0 = {0};
	union hqmv2_sys_smon_cfg1 r1 = {0};
	union hqmv2_sys_smon_compare0 r2 = {0};
	union hqmv2_sys_smon_compare1 r3 = {0};
	union hqmv2_sys_smon_comp_mask0 r4 = {0};
	union hqmv2_sys_smon_comp_mask1 r5 = {0};
	union hqmv2_sys_smon_activitycntr0 r6 = {0};
	union hqmv2_sys_smon_activitycntr1 r7 = {0};
	union hqmv2_sys_smon_tmr r8 = {0};
	union hqmv2_sys_smon_max_tmr r9 = {0};

	/* Disable the SMON before configuring it */
	HQMV2_CSR_WR(hw, HQMV2_SYS_SMON_CFG0, r0.val);

	r1.field.mode0 = cfg->mode[0];
	r1.field.mode1 = cfg->mode[1];

	HQMV2_CSR_WR(hw, HQMV2_SYS_SMON_CFG1, r1.val);

	r2.field.compare0 = cfg->cmp[0];

	HQMV2_CSR_WR(hw, HQMV2_SYS_SMON_COMPARE0, r2.val);

	r3.field.compare1 = cfg->cmp[1];

	HQMV2_CSR_WR(hw, HQMV2_SYS_SMON_COMPARE1, r3.val);

	r4.field.comp_mask0 = cfg->mask[0];

	HQMV2_CSR_WR(hw, HQMV2_SYS_SMON_COMP_MASK0, r4.val);

	r5.field.comp_mask1 = cfg->mask[1];

	HQMV2_CSR_WR(hw, HQMV2_SYS_SMON_COMP_MASK1, r5.val);

	r6.field.counter0 = cfg->cnt[0];

	HQMV2_CSR_WR(hw, HQMV2_SYS_SMON_ACTIVITYCNTR0, r6.val);

	r7.field.counter1 = cfg->cnt[1];

	HQMV2_CSR_WR(hw, HQMV2_SYS_SMON_ACTIVITYCNTR1, r7.val);

	r8.field.timer_val = cfg->timer;

	HQMV2_CSR_WR(hw, HQMV2_SYS_SMON_TMR, r8.val);

	r9.field.maxvalue = cfg->max_timer;

	HQMV2_CSR_WR(hw, HQMV2_SYS_SMON_MAX_TMR, r9.val);

	r0.field.smon_enable = 1;
	r0.field.smon0_function = cfg->fn[0];
	r0.field.smon0_function_compare = cfg->fn_cmp[0];
	r0.field.smon1_function = cfg->fn[1];
	r0.field.smon1_function_compare = cfg->fn_cmp[1];
	r0.field.smon_mode = cfg->smon_mode;
	r0.field.stopcounterovfl = cfg->stop_counter_mode;
	r0.field.stoptimerovfl = cfg->stop_timer_mode;

	/* Enable the SMON */
	HQMV2_CSR_WR(hw, HQMV2_SYS_SMON_CFG0, r0.val);
}

static void hqmv2_smon_setup_chp_perf_smon(struct hqmv2_hw *hw,
					   struct hqmv2_smon_cfg *cfg)
{
	union hqmv2_chp_smon_cfg0 r0 = {0};
	union hqmv2_chp_smon_cfg1 r1 = {0};
	union hqmv2_chp_smon_compare0 r2 = {0};
	union hqmv2_chp_smon_compare1 r3 = {0};
	union hqmv2_chp_smon_cntr0 r4 = {0};
	union hqmv2_chp_smon_cntr1 r5 = {0};
	union hqmv2_chp_smon_tmr r6 = {0};
	union hqmv2_chp_smon_max_tmr r7 = {0};

	/* Disable the SMON before configuring it */
	HQMV2_CSR_WR(hw, HQMV2_CHP_SMON_CFG0, r0.val);

	r1.field.mode0 = cfg->mode[0];
	r1.field.mode1 = cfg->mode[1];

	HQMV2_CSR_WR(hw, HQMV2_CHP_SMON_CFG1, r1.val);

	r2.field.compare0 = cfg->cmp[0];

	HQMV2_CSR_WR(hw, HQMV2_CHP_SMON_COMPARE0, r2.val);

	r3.field.compare1 = cfg->cmp[1];

	HQMV2_CSR_WR(hw, HQMV2_CHP_SMON_COMPARE1, r3.val);

	r4.field.counter0 = cfg->cnt[0];

	HQMV2_CSR_WR(hw, HQMV2_CHP_SMON_CNTR0, r4.val);

	r5.field.counter1 = cfg->cnt[1];

	HQMV2_CSR_WR(hw, HQMV2_CHP_SMON_CNTR1, r5.val);

	r6.field.timer = cfg->timer;

	HQMV2_CSR_WR(hw, HQMV2_CHP_SMON_TMR, r6.val);

	r7.field.maxvalue = cfg->max_timer;

	HQMV2_CSR_WR(hw, HQMV2_CHP_SMON_MAX_TMR, r7.val);

	r0.field.smon_enable = 1;
	r0.field.smon0_function = cfg->fn[0];
	r0.field.smon0_function_compare = cfg->fn_cmp[0];
	r0.field.smon1_function = cfg->fn[1];
	r0.field.smon1_function_compare = cfg->fn_cmp[1];
	r0.field.smon_mode = cfg->smon_mode;
	r0.field.stopcounterovfl = cfg->stop_counter_mode;
	r0.field.stoptimerovfl = cfg->stop_timer_mode;

	/* Enable the SMON */
	HQMV2_CSR_WR(hw, HQMV2_CHP_SMON_CFG0, r0.val);
}

void hqmv2_smon_setup_sys_iosf_measurements(struct hqmv2_hw *hw,
					    u32 max)
{
	struct hqmv2_smon_cfg sys_pm_cfg = DEFAULT_SMON_CFG;

	sys_pm_cfg.mode[1] = 1;
	sys_pm_cfg.max_timer = max;
	sys_pm_cfg.stop_timer_mode = 1;
	sys_pm_cfg.stop_counter_mode = 1;

	hqmv2_smon_setup_sys_perf_smon(hw, &sys_pm_cfg);
}

void hqmv2_smon_setup_chp_ing_egr_measurements(struct hqmv2_hw *hw,
					       u32 max)
{
	struct hqmv2_smon_cfg chp_pm_cfg = DEFAULT_SMON_CFG;

	chp_pm_cfg.mode[1] = 1;
	chp_pm_cfg.max_timer = max;
	chp_pm_cfg.stop_timer_mode = 1;
	chp_pm_cfg.stop_counter_mode = 1;

	hqmv2_smon_setup_chp_perf_smon(hw, &chp_pm_cfg);
}

void hqmv2_smon_setup_chp_pp_count_measurement(struct hqmv2_hw *hw,
					       u32 max)
{
	struct hqmv2_smon_cfg chp_pm_cfg = DEFAULT_SMON_CFG;

	chp_pm_cfg.mode[0] = 2;
	chp_pm_cfg.mode[1] = 2;
	chp_pm_cfg.max_timer = max;
	chp_pm_cfg.stop_timer_mode = 1;
	chp_pm_cfg.stop_counter_mode = 1;

	hqmv2_smon_setup_chp_perf_smon(hw, &chp_pm_cfg);
}

void hqmv2_smon_setup_chp_avg_measurement(struct hqmv2_hw *hw,
					  enum hqmv2_chp_smon_component type,
					  u32 max)
{
	struct hqmv2_smon_cfg chp_pm_cfg = DEFAULT_SMON_CFG;

	chp_pm_cfg.mode[0] = type;
	chp_pm_cfg.mode[1] = type;
	chp_pm_cfg.max_timer = max;
	chp_pm_cfg.stop_timer_mode = 1;
	chp_pm_cfg.stop_counter_mode = 1;
	chp_pm_cfg.smon_mode = 3;

	hqmv2_smon_setup_chp_perf_smon(hw, &chp_pm_cfg);
}

void hqmv2_smon_setup_chp_hcw_measurements(struct hqmv2_hw *hw,
					   enum hqmv2_chp_smon_hcw_type type_0,
					   enum hqmv2_chp_smon_hcw_type type_1,
					   u32 max)
{
	struct hqmv2_smon_cfg chp_pm_cfg = DEFAULT_SMON_CFG;

	chp_pm_cfg.mode[0] = 0;
	chp_pm_cfg.mode[1] = 0;
	chp_pm_cfg.cmp[0] = type_0;
	chp_pm_cfg.cmp[1] = type_1;
	chp_pm_cfg.fn_cmp[0] = 1;
	chp_pm_cfg.fn_cmp[1] = 1;
	chp_pm_cfg.max_timer = max;
	chp_pm_cfg.stop_timer_mode = 1;
	chp_pm_cfg.stop_counter_mode = 1;

	hqmv2_smon_setup_chp_perf_smon(hw, &chp_pm_cfg);
}

void hqmv2_smon_read_sys_perf_counter(struct hqmv2_hw *hw,
				      enum hqmv2_smon_meas_type type,
				      u32 counter[2])
{
	union hqmv2_sys_smon_cfg0 r0 = {0};
	union hqmv2_sys_smon_activitycntr0 r1;
	union hqmv2_sys_smon_activitycntr1 r2;
	union hqmv2_sys_smon_tmr r3;

	/* Disable the SMON */
	HQMV2_CSR_WR(hw, HQMV2_SYS_SMON_CFG0, r0.val);

	r1.val = HQMV2_CSR_RD(hw, HQMV2_SYS_SMON_ACTIVITYCNTR0);
	r2.val = HQMV2_CSR_RD(hw, HQMV2_SYS_SMON_ACTIVITYCNTR1);
	r3.val = HQMV2_CSR_RD(hw, HQMV2_SYS_SMON_TMR);

	if (type == SMON_MEASURE_CNT) {
		counter[0] = r1.val;
		counter[1] = r2.val;
	} else if (type == SMON_MEASURE_AVG) {
		counter[0] = (r1.val) ? (r2.val / r1.val) : 0;
	}
}

void hqmv2_smon_read_chp_perf_counter(struct hqmv2_hw *hw,
				      enum hqmv2_smon_meas_type type,
				      u32 counter[2])
{
	union hqmv2_chp_smon_cfg0 r0 = {0};
	union hqmv2_chp_smon_cntr0 r1;
	union hqmv2_chp_smon_cntr1 r2;
	union hqmv2_chp_smon_tmr r3;
	u64 counter0_overflow = 0;
	u64 counter1_overflow = 0;
	u64 count64[2];

	r0.val = HQMV2_CSR_RD(hw, HQMV2_CHP_SMON_CFG0);

	if (r0.field.statcounter0ovfl)
		counter0_overflow = 0x100000000;
	if (r0.field.statcounter1ovfl)
		counter1_overflow = 0x100000000;

	/* Disable the SMON */
	HQMV2_CSR_WR(hw, HQMV2_CHP_SMON_CFG0, r0.val);

	r1.val = HQMV2_CSR_RD(hw, HQMV2_CHP_SMON_CNTR0);
	r2.val = HQMV2_CSR_RD(hw, HQMV2_CHP_SMON_CNTR1);
	r3.val = HQMV2_CSR_RD(hw, HQMV2_CHP_SMON_TMR);

	if (type == SMON_MEASURE_CNT) {
		counter[0] = r1.val;
		counter[1] = r2.val;
	} else if (type == SMON_MEASURE_AVG) {
		count64[0] = ((u64)r1.val) + counter0_overflow;
		count64[1] = ((u64)r2.val) + counter1_overflow;
		counter[0] = (count64[0]) ? (count64[1] / count64[0]) : 0;
	}
}
