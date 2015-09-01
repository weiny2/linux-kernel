#ifndef _LINK_H
#define _LINK_H
/*
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2015 Intel Corporation.
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
 * Copyright(c) 2015 Intel Corporation.
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

/*
 * This file contains all of the code that is specific to
 * Link Negotiation and Initialization(LNI)
 */

/* CRK_CFG_PORT_CONFIG logical link states */
#define LSTATE_DOWN    0x1
#define LSTATE_INIT    0x2
#define LSTATE_ARMED   0x3
#define LSTATE_ACTIVE  0x4

/* CRK8051_STS_CUR_STATE port values (physical link states) */
#define PLS_DISABLED			   0x30
#define PLS_OFFLINE				   0x90
#define PLS_OFFLINE_QUIET			   0x90
#define PLS_OFFLINE_PLANNED_DOWN_INFORM	   0x91
#define PLS_OFFLINE_READY_TO_QUIET_LT	   0x92
#define PLS_OFFLINE_REPORT_FAILURE		   0x93
#define PLS_OFFLINE_READY_TO_QUIET_BCC	   0x94
#define PLS_POLLING				   0x20
#define PLS_POLLING_QUIET			   0x20
#define PLS_POLLING_ACTIVE			   0x21
#define PLS_CONFIGPHY			   0x40
#define PLS_CONFIGPHY_DEBOUCE		   0x40
#define PLS_CONFIGPHY_ESTCOMM		   0x41
#define PLS_CONFIGPHY_ESTCOMM_TXRX_HUNT	   0x42
#define PLS_CONFIGPHY_ESTcOMM_LOCAL_COMPLETE   0x43
#define PLS_CONFIGPHY_OPTEQ			   0x44
#define PLS_CONFIGPHY_OPTEQ_OPTIMIZING	   0x44
#define PLS_CONFIGPHY_OPTEQ_LOCAL_COMPLETE	   0x45
#define PLS_CONFIGPHY_VERIFYCAP		   0x46
#define PLS_CONFIGPHY_VERIFYCAP_EXCHANGE	   0x46
#define PLS_CONFIGPHY_VERIFYCAP_LOCAL_COMPLETE 0x47
#define PLS_CONFIGLT			   0x48
#define PLS_CONFIGLT_CONFIGURE		   0x48
#define PLS_CONFIGLT_LINK_TRANSFER_ACTIVE	   0x49
#define PLS_LINKUP				   0x50
#define PLS_PHYTEST				   0xB0
#define PLS_INTERNAL_SERDES_LOOPBACK	   0xe1
#define PLS_QUICK_LINKUP			   0xe2

/* CRK_DC8051_CFG_HOST_CMD_0.REQ_TYPE - 8051 host commands */
#define HCMD_LOAD_CONFIG_DATA  0x01
#define HCMD_READ_CONFIG_DATA  0x02
#define HCMD_CHANGE_PHY_STATE  0x03
#define HCMD_SEND_LCB_IDLE_MSG 0x04
#define HCMD_MISC		   0x05
#define HCMD_READ_LCB_IDLE_MSG 0x06
#define HCMD_READ_LCB_CSR      0x07
#define HCMD_INTERFACE_TEST	   0xff

/* CRK_DC8051_CFG_HOST_CMD_1.RETURN_CODE - 8051 host command return */
#define HCMD_INVALID 0x1
#define HCMD_SUCCESS 0x2
#define HCMD_ONGOING 0xff

/* timeouts */
#define TIMEOUT_8051_START 5000         /* 8051 start timeout, in ms */
#define CRK8051_COMMAND_TIMEOUT 20000	/* 8051 command timeout, in ms */

extern bool quick_linkup; /* skip VarifyCap and Config* state. */

u64 read_fzc_csr(const struct hfi_pportdata *ppd, u32 offset);
void write_fzc_csr(const struct hfi_pportdata *ppd, u32 offset, u64 value);
u64 read_8051_csr(const struct hfi_pportdata *ppd, u32 offset);
void write_8051_csr(const struct hfi_pportdata *ppd, u32 offset, u64 value);
void hfi_8051_reset(const struct hfi_pportdata *ppd);
int hfi_set_link_state(struct hfi_pportdata *ppd, u32 state);
int hfi_start_link(struct hfi_pportdata *ppd);

#endif /* _LINK_H */
