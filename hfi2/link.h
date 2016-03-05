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
/*
 * This file contains all of the code that is specific to
 * Link Negotiation and Initialization(LNI)
 */
#ifndef _LINK_H
#define _LINK_H
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

/* DC_DC8051_DBG_ERR_INFO_SET_BY_8051.HOST_MSG - host message flags */
#define HOST_REQ_DONE	   (1 << 0)
#define BC_PWR_MGM_MSG	   (1 << 1)
#define BC_SMA_MSG		   (1 << 2)
#define BC_BCC_UNKOWN_MSG	   (1 << 3)
#define BC_IDLE_UNKNOWN_MSG	   (1 << 4)
#define EXT_DEVICE_CFG_REQ	   (1 << 5)
#define VERIFY_CAP_FRAME	   (1 << 6)
#define LINKUP_ACHIEVED	   (1 << 7)
#define LINK_GOING_DOWN	   (1 << 8)
#define LINK_WIDTH_DOWNGRADED  (1 << 9)

/* idle flit message types */
#define IDLE_PHYSICAL_LINK_MGMT 0x1
#define IDLE_CRU		    0x2
#define IDLE_SMA		    0x3
#define IDLE_POWER_MGMT	    0x4

/* idle flit message send fields (both send and read) */
#define IDLE_PAYLOAD_MASK 0xffffffffffull /* 40 bits */
#define IDLE_PAYLOAD_SHIFT 8
#define IDLE_MSG_TYPE_MASK 0xf
#define IDLE_MSG_TYPE_SHIFT 0

/* 8051 general register Field IDs */
#define HOST_INT_MSG_MASK		0x03

/*
 * 8051 firmware registers
 */
#define NUM_GENERAL_FIELDS 0x17
#define NUM_LANE_FIELDS    0x8

/* 8051 general register Field IDs */
#define LINK_OPTIMIZATION_SETTINGS   0x00
#define LINK_TUNING_PARAMETERS	     0x02
#define DC_HOST_COMM_SETTINGS	     0x03
#define TX_SETTINGS		     0x06
#define VERIFY_CAP_LOCAL_PHY	     0x07
#define VERIFY_CAP_LOCAL_FABRIC	     0x08
#define VERIFY_CAP_LOCAL_LINK_WIDTH  0x09
#define LOCAL_DEVICE_ID		     0x0a
#define LOCAL_LNI_INFO		     0x0c
#define REMOTE_LNI_INFO              0x0d
#define MISC_STATUS		     0x0e
#define VERIFY_CAP_REMOTE_PHY	     0x0f
#define VERIFY_CAP_REMOTE_FABRIC     0x10
#define VERIFY_CAP_REMOTE_LINK_WIDTH 0x11
#define LAST_LOCAL_STATE_COMPLETE    0x12
#define LAST_REMOTE_STATE_COMPLETE   0x13
#define LINK_QUALITY_INFO            0x14
#define REMOTE_DEVICE_ID	     0x15

/* Lane ID for general configuration registers */
#define GENERAL_CONFIG 4

/* LOAD_DATA 8051 command shifts and fields */
#define LOAD_DATA_FIELD_ID_SHIFT 40
#define LOAD_DATA_FIELD_ID_MASK 0xfull
#define LOAD_DATA_LANE_ID_SHIFT 32
#define LOAD_DATA_LANE_ID_MASK 0xfull
#define LOAD_DATA_DATA_SHIFT  0x0
#define LOAD_DATA_DATA_MASK   0xffffffffull

/* verify capability local link width fields */
#define LINK_WIDTH_SHIFT 0		/* also for remote link width */
#define LOCAL_LINK_WIDTH_MASK 0xffff
#define REMOTE_LINK_WIDTH_MASK 0xff
#define LOCAL_FLAG_BITS_SHIFT 16
#define LOCAL_FLAG_BITS_MASK 0xff
#define MISC_CONFIG_BITS_SHIFT 24
#define MISC_CONFIG_BITS_MASK 0xff

/* LOCAL_DEVICE_ID fields */
#define LOCAL_DEVICE_REV_SHIFT 0
#define LOCAL_DEVICE_REV_MASK 0xff
#define LOCAL_DEVICE_ID_SHIFT 8
#define LOCAL_DEVICE_ID_MASK 0xffff

/* REMOTE_DEVICE_ID fields */
#define REMOTE_DEVICE_REV_SHIFT 0
#define REMOTE_DEVICE_REV_MASK 0xff
#define REMOTE_DEVICE_ID_SHIFT 8
#define REMOTE_DEVICE_ID_MASK 0xffff

/* verify capability remote link width fields */
#define REMOTE_TX_RATE_SHIFT 16
#define REMOTE_TX_RATE_MASK 0xff

/* mask, shift for reading 'mgmt_enabled' value from REMOTE_LNI_INFO field */
#define MGMT_ALLOWED_SHIFT 23
#define MGMT_ALLOWED_MASK 0x1

#define LINK_QUALITY_SHIFT 24
#define LINK_QUALITY_MASK 0x7

/* New on FXR, not defined in 4.3 kernel */
#define OPA_LINK_SPEED_32G 3 /* 32.2265625 32Gbps */

/* FXR link speed bit */
#define HFI2_LINK_SPEED_32G 0x04 /* 32.2265625 32Gbps */
#define HFI2_LINK_SPEED_25G 0x02 /* 25.7815 25Gbps */
#define HFI2_LINK_SPEED_12_5G 0x01 /* 12.5Gbps */

/* misc status version fields */
#define STS_FM_VERSION_A_SHIFT 16
#define STS_FM_VERSION_A_MASK  0xff
#define STS_FM_VERSION_B_SHIFT 24
#define STS_FM_VERSION_B_MASK  0xff

/* timeouts */
#define TIMEOUT_8051_START 5000         /* 8051 start timeout, in ms */
#define CRK8051_COMMAND_TIMEOUT 20000	/* 8051 command timeout, in ms */

/* interrupt vector number */
#define HFI2_MNH_ERROR 256
#define HFI2_FZC0_ERROR 257
#define HFI2_FZC1_ERROR 258

u64 read_fzc_csr(const struct hfi_pportdata *ppd, u32 offset);
void write_fzc_csr(const struct hfi_pportdata *ppd, u32 offset, u64 value);
u64 read_8051_csr(const struct hfi_pportdata *ppd, u32 offset);
void write_8051_csr(const struct hfi_pportdata *ppd, u32 offset, u64 value);
u8 hfi_ibphys_portstate(struct hfi_pportdata *ppd);
void hfi_set_link_down_reason(struct hfi_pportdata *ppd, u8 lcl_reason,
			  u8 neigh_reason, u8 rem_reason);
int hfi2_wait_logical_linkstate(struct hfi_pportdata *ppd, u32 state,
								  int msecs);
int hfi_send_idle_sma(struct hfi_pportdata *ppd, u64 message);
int hfi2_cfg_link_intr_vector(struct hfi_devdata *dd);
int hfi2_enable_8051_intr(struct hfi_pportdata *ppd);
int hfi2_disable_8051_intr(struct hfi_pportdata *ppd);
void hfi2_pport_link_uninit(struct hfi_devdata *dd);
int hfi2_pport_link_init(struct hfi_devdata *dd);
int hfi_set_link_state(struct hfi_pportdata *ppd, u32 state);
int hfi_start_link(struct hfi_pportdata *ppd);
u16 hfi_cap_to_port_ltp(u16 cap);
u16 hfi_port_ltp_to_cap(u16 port_ltp);
void hfi_read_link_quality(struct hfi_pportdata *ppd, u8 *link_quality);


#endif /* _LINK_H */
