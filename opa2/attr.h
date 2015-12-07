/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Intel Corporation nor the names of its
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
 */

/*
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */

#ifndef _ATTR_H
#define _ATTR_H

#define OPA_CONG_LOG_ELEMS	96
#define PHYSPORTSTATE_SHIFT	4
#define NNORMAL_SHIFT		4
#define SM_CONFIG_SHIFT		5
#define HFI_PKEY_BLOCKS_AVAIL	(HFI_MAX_PKEYS / OPA_PARTITION_TABLE_BLK_SIZE)

/* the reset value from the FM is supposed to be 0xffff, handle both */
#define HFI_LINK_WIDTH_RESET_OLD 0x0fff
#define HFI_LINK_WIDTH_RESET 0xffff

#define HFI_IB_CC_TABLE_CAP_DEFAULT	31
#define HFI_IB_CCT_ENTRIES		64
#define HFI_CC_TABLE_SHADOW_MAX \
	(HFI_IB_CC_TABLE_CAP_DEFAULT * HFI_IB_CCT_ENTRIES)

/* port control flags */
#define HFI_IB_CC_CCS_PC_SL_BASED 0x01

struct opa_congestion_info_attr {
	__be16 congestion_info;
	u8 control_table_cap;	/* Multiple of 64 entry unit CCTs */
	u8 congestion_log_length;
} __packed;

struct opa_congestion_setting_entry {
	u8 ccti_increase;
	u8 reserved;
	__be16 ccti_timer;
	u8 trigger_threshold;
	u8 ccti_min; /* min CCTI for cc table */
} __packed;

struct opa_congestion_setting_entry_shadow {
	u8 ccti_increase;
	u8 reserved;
	u16 ccti_timer;
	u8 trigger_threshold;
	u8 ccti_min; /* min CCTI for cc table */
} __packed;

struct ib_cc_table_entry_shadow {
	u16 entry; /* shift:2, multiplier:14 */
};

struct ib_cc_table_entry {
	__be16 entry; /* shift:2, multiplier:14 */
};

struct ib_cc_table_attr {
	__be16 ccti_limit; /* max CCTI for cc table */
	struct ib_cc_table_entry ccti_entries[HFI_IB_CCT_ENTRIES];
} __packed;

struct cc_table_shadow {
	u16 ccti_limit; /* max CCTI for cc table */
	struct ib_cc_table_entry_shadow entries[HFI_CC_TABLE_SHADOW_MAX];
} __packed;

struct opa_congestion_setting_attr {
	__be32 control_map;
	__be16 port_control;
	struct opa_congestion_setting_entry entries[OPA_MAX_SLS];
} __packed;

struct opa_congestion_setting_attr_shadow {
	u32 control_map;
	u16 port_control;
	struct opa_congestion_setting_entry_shadow entries[OPA_MAX_SLS];
} __packed;

/*
 * struct cc_state combines the (active) per-port congestion control
 * table, and the (active) per-SL congestion settings. cc_state data
 * may need to be read in code paths that we want to be fast, so it
 * is an RCU protected structure.
 */
struct cc_state {
	struct rcu_head rcu;
	struct cc_table_shadow cct;
	struct opa_congestion_setting_attr_shadow cong_setting;
};
#endif	/* _ATTR_H */
