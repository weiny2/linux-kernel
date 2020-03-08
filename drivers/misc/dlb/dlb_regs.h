/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
 * Copyright(c) 2016-2020 Intel Corporation
 */

#ifndef __DLB_REGS_H
#define __DLB_REGS_H

#include <linux/types.h>

#define DLB_SYS_TOTAL_VAS 0x124
#define DLB_SYS_TOTAL_VAS_RST 0x20
union dlb_sys_total_vas {
	struct {
		u32 total_vas : 32;
	} field;
	u32 val;
};

#define DLB_DP_DIR_CSR_CTRL 0x38000018
#define DLB_DP_DIR_CSR_CTRL_RST 0xc0000000
union dlb_dp_dir_csr_ctrl {
	struct {
		u32 cfg_int_dis : 1;
		u32 cfg_int_dis_sbe : 1;
		u32 cfg_int_dis_mbe : 1;
		u32 spare0 : 27;
		u32 cfg_vasr_dis : 1;
		u32 cfg_int_dis_synd : 1;
	} field;
	u32 val;
};

#endif /* __DLB_REGS_H */
