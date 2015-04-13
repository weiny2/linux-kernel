#ifndef _WFR_H
#define _WFR_H
/*
 * Copyright (c) 2013-2015 Intel Corporation.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * This file contains all of the defines that is specific to the WFR chip
 */

/* sizes */
#define WFR_CCE_NUM_MSIX_VECTORS 256
#define WFR_CCE_NUM_INT_CSRS 12
#define WFR_CCE_NUM_INT_MAP_CSRS 96
#define WFR_NUM_INTERRUPT_SOURCES 768
#define WFR_RXE_NUM_CONTEXTS 160
#define WFR_RXE_PER_CONTEXT_SIZE 0x1000	/* 4k */
#define WFR_RXE_NUM_TID_FLOWS 32
#define WFR_RXE_NUM_DATA_VL 8
#define WFR_TXE_NUM_CONTEXTS 160
#define WFR_TXE_NUM_SDMA_ENGINES 16
#define WFR_NUM_CONTEXTS_PER_SET 8
#define WFR_VL_ARB_HIGH_PRIO_TABLE_SIZE 16
#define WFR_VL_ARB_LOW_PRIO_TABLE_SIZE 16
#define WFR_TXE_NUM_32_BIT_COUNTER 7
#define WFR_TXE_NUM_64_BIT_COUNTER 30
#define WFR_TXE_NUM_DATA_VL 8
#define WFR_TXE_PIO_SIZE (32 * 0x100000)	/* 32 MB */
#define WFR_PIO_BLOCK_SIZE 64			/* bytes */
#define WFR_SDMA_BLOCK_SIZE 64			/* bytes */
#define WFR_RCV_BUF_BLOCK_SIZE 64               /* bytes */
#define WFR_PIO_CMASK 0x7ff	/* counter mask for free and fill counters */
#define WFR_MAX_EAGER_ENTRIES    2048	/* max receive eager entries */
#define WFR_MAX_TID_PAIR_ENTRIES 1024	/* max receive expected pairs */
/* Virtual? Allocation Unit, defined as AU = 8*2^vAU, 64 bytes, AU is fixed
   at 64 bytes for all generation one devices */
#define WFR_CM_VAU 3
/* WFR link credit count, AKA receive buffer depth (RBUF_DEPTH) */
#define WFR_CM_GLOBAL_CREDITS 0x940
/* Number of PKey entries in the HW */
#define WFR_MAX_PKEY_VALUES 16

#include "include/wfr/wfr_core_defs.h"
#include "include/wfr/wfr_cce_defs.h"
#include "include/wfr/wfr_rxe_defs.h"
#include "include/wfr/wfr_txe_defs.h"
#include "include/wfr/wfr_misc_defs.h"
#include "include/wfr/wfr_asic_defs.h"
#include "include/wfr/dc_8051_csrs_defs.h"
#include "include/wfr/dcc_csrs_defs.h"
#include "include/wfr/dc_lcb_csrs_defs.h"

/* not defined in wfr_core_defs.h */
#define WFR_RXE_PER_CONTEXT_USER_OFFSET 0x0300000
#define WFR_RXE_PER_CONTEXT_USER   (WFR_RXE + WFR_RXE_PER_CONTEXT_USER_OFFSET)

#define WFR_TXE_PIO_SEND_OFFSET 0x0800000
#define WFR_TXE_PIO_SEND (WFR_TXE + WFR_TXE_PIO_SEND_OFFSET)

/* PBC flags */
#define WFR_PBC_INTR		(1ull << 31)
#define WFR_PBC_DC_INFO_SHIFT	(30)
#define WFR_PBC_DC_INFO		(1ull << WFR_PBC_DC_INFO_SHIFT)
#define WFR_PBC_TEST_EBP	(1ull << 29)
#define WFR_PBC_PACKET_BYPASS	(1ull << 28)
#define WFR_PBC_CREDIT_RETURN	(1ull << 25)
#define WFR_PBC_INSERT_BYPASS_ICRC (1ull << 24)
#define WFR_PBC_TEST_BAD_ICRC	(1ull << 23)
#define WFR_PBC_FECN		(1ull << 22)

/* PbcInsertHcrc field settings */
#define WFR_PBC_IHCRC_LKDETH 0x0	/* insert @ local KDETH offset */
#define WFR_PBC_IHCRC_GKDETH 0x1	/* insert @ global KDETH offset */
#define WFR_PBC_IHCRC_NONE   0x2	/* no HCRC inserted */

/* PBC fields */
#define WFR_PBC_STATIC_RATE_CONTROL_COUNT_SHIFT 32
#define WFR_PBC_STATIC_RATE_CONTROL_COUNT_MASK 0xffffull
#define WFR_PBC_STATIC_RATE_CONTROL_COUNT_SMASK \
	(WFR_PBC_STATIC_RATE_CONTROL_COUNT_MASK << \
	WFR_PBC_STATIC_RATE_CONTROL_COUNT_SHIFT)

#define WFR_PBC_INSERT_HCRC_SHIFT 26
#define WFR_PBC_INSERT_HCRC_MASK 0x3ull
#define WFR_PBC_INSERT_HCRC_SMASK \
	(WFR_PBC_INSERT_HCRC_MASK << WFR_PBC_INSERT_HCRC_SHIFT)

#define WFR_PBC_VL_SHIFT 12
#define WFR_PBC_VL_MASK 0xfull
#define WFR_PBC_VL_SMASK (WFR_PBC_VL_MASK << WFR_PBC_VL_SHIFT)

#define WFR_PBC_LENGTH_DWS_SHIFT 0
#define WFR_PBC_LENGTH_DWS_MASK 0xfffull
#define WFR_PBC_LENGTH_DWS_SMASK \
	(WFR_PBC_LENGTH_DWS_MASK << WFR_PBC_LENGTH_DWS_SHIFT)

/* Credit Return Fields */
#define WFR_CR_COUNTER_SHIFT 0
#define WFR_CR_COUNTER_MASK 0x7ffull
#define WFR_CR_COUNTER_SMASK (WFR_CR_COUNTER_MASK << WFR_CR_COUNTER_SHIFT)

#define WFR_CR_STATUS_SHIFT 11
#define WFR_CR_STATUS_MASK 0x1ull
#define WFR_CR_STATUS_SMASK (WFR_CR_STATUS_MASK << WFR_CR_STATUS_SHIFT)

#define WFR_CR_CREDIT_RETURN_DUE_TO_PBC_SHIFT 12
#define WFR_CR_CREDIT_RETURN_DUE_TO_PBC_MASK 0x1ull
#define WFR_CR_CREDIT_RETURN_DUE_TO_PBC_SMASK \
	(WFR_CR_CREDIT_RETURN_DUE_TO_PBC_MASK << \
	WFR_CR_CREDIT_RETURN_DUE_TO_PBC_SHIFT)

#define WFR_CR_CREDIT_RETURN_DUE_TO_THRESHOLD_SHIFT 13
#define WFR_CR_CREDIT_RETURN_DUE_TO_THRESHOLD_MASK 0x1ull
#define WFR_CR_CREDIT_RETURN_DUE_TO_THRESHOLD_SMASK \
	(WFR_CR_CREDIT_RETURN_DUE_TO_THRESHOLD_MASK << \
	WFR_CR_CREDIT_RETURN_DUE_TO_THRESHOLD_SHIFT)

#define WFR_CR_CREDIT_RETURN_DUE_TO_ERR_SHIFT 14
#define WFR_CR_CREDIT_RETURN_DUE_TO_ERR_MASK 0x1ull
#define WFR_CR_CREDIT_RETURN_DUE_TO_ERR_SMASK \
	(WFR_CR_CREDIT_RETURN_DUE_TO_ERR_MASK << \
	WFR_CR_CREDIT_RETURN_DUE_TO_ERR_SHIFT)

#define WFR_CR_CREDIT_RETURN_DUE_TO_FORCE_SHIFT 15
#define WFR_CR_CREDIT_RETURN_DUE_TO_FORCE_MASK 0x1ull
#define WFR_CR_CREDIT_RETURN_DUE_TO_FORCE_SMASK \
	(WFR_CR_CREDIT_RETURN_DUE_TO_FORCE_MASK << \
	WFR_CR_CREDIT_RETURN_DUE_TO_FORCE_SHIFT)

/* interrupt source numbers */
#define WFR_IS_GENERAL_ERR_START	  0
#define WFR_IS_SDMAENG_ERR_START	 16
#define WFR_IS_SENDCTXT_ERR_START	 32
#define WFR_IS_SDMA_START		192 /* includes SDmaProgress,SDmaIdle */
#define WFR_IS_VARIOUS_START		240
#define WFR_IS_DC_START			248
#define WFR_IS_RCVAVAIL_START		256
#define WFR_IS_RCVURGENT_START		416
#define WFR_IS_SENDCREDIT_START		576
#define WFR_IS_RESERVED_START		736
#define WFR_IS_MAX_SOURCES		768

/* derived interrupt source values */
#define WFR_IS_GENERAL_ERR_END		WFR_IS_SDMAENG_ERR_START
#define WFR_IS_SDMAENG_ERR_END		WFR_IS_SENDCTXT_ERR_START
#define WFR_IS_SENDCTXT_ERR_END		WFR_IS_SDMA_START
#define WFR_IS_SDMA_END			WFR_IS_VARIOUS_START
#define WFR_IS_VARIOUS_END		WFR_IS_DC_START
#define WFR_IS_DC_END			WFR_IS_RCVAVAIL_START
#define WFR_IS_RCVAVAIL_END		WFR_IS_RCVURGENT_START
#define WFR_IS_RCVURGENT_END		WFR_IS_SENDCREDIT_START
#define WFR_IS_SENDCREDIT_END		WFR_IS_RESERVED_START
#define WFR_IS_RESERVED_END		WFR_IS_MAX_SOURCES

/* absolute interrupt numbers for QSFP1Int and QSFP2Int */
#define WFR_QSFP1_INT		242
#define WFR_QSFP2_INT		243

/* DCC_CFG_PORT_CONFIG logical link states */
#define WFR_LSTATE_DOWN    0x1
#define WFR_LSTATE_INIT    0x2
#define WFR_LSTATE_ARMED   0x3
#define WFR_LSTATE_ACTIVE  0x4

/* DC8051_STS_CUR_STATE port values (physical link states) */
#define WFR_PLS_DISABLED			   0x30
#define WFR_PLS_OFFLINE				   0x90
#define WFR_PLS_OFFLINE_QUIET			   0x90
#define WFR_PLS_OFFLINE_PLANNED_DOWN_INFORM	   0x91
#define WFR_PLS_OFFLINE_READY_TO_QUIET_LT	   0x92
#define WFR_PLS_OFFLINE_REPORT_FAILURE		   0x93
#define WFR_PLS_OFFLINE_READY_TO_QUIET_BCC	   0x94
#define WFR_PLS_POLLING				   0x20
#define WFR_PLS_POLLING_QUIET			   0x20
#define WFR_PLS_POLLING_ACTIVE			   0x21
#define WFR_PLS_CONFIGPHY			   0x40
#define WFR_PLS_CONFIGPHY_DEBOUCE		   0x40
#define WFR_PLS_CONFIGPHY_ESTCOMM		   0x41
#define WFR_PLS_CONFIGPHY_ESTCOMM_TXRX_HUNT	   0x42
#define WFR_PLS_CONFIGPHY_ESTcOMM_LOCAL_COMPLETE   0x43
#define WFR_PLS_CONFIGPHY_OPTEQ			   0x44
#define WFR_PLS_CONFIGPHY_OPTEQ_OPTIMIZING	   0x44
#define WFR_PLS_CONFIGPHY_OPTEQ_LOCAL_COMPLETE	   0x45
#define WFR_PLS_CONFIGPHY_VERIFYCAP		   0x46
#define WFR_PLS_CONFIGPHY_VERIFYCAP_EXCHANGE	   0x46
#define WFR_PLS_CONFIGPHY_VERIFYCAP_LOCAL_COMPLETE 0x47
#define WFR_PLS_CONFIGLT			   0x48
#define WFR_PLS_CONFIGLT_CONFIGURE		   0x48
#define WFR_PLS_CONFIGLT_LINK_TRANSFER_ACTIVE	   0x49
#define WFR_PLS_LINKUP				   0x50
#define WFR_PLS_PHYTEST				   0xB0
#define WFR_PLS_INTERNAL_SERDES_LOOPBACK	   0xe1
#define WFR_PLS_QUICK_LINKUP			   0xe2

/* DC_DC8051_CFG_HOST_CMD_0.REQ_TYPE - 8051 host commands */
#define WFR_HCMD_LOAD_CONFIG_DATA  0x01
#define WFR_HCMD_READ_CONFIG_DATA  0x02
#define WFR_HCMD_CHANGE_PHY_STATE  0x03
#define WFR_HCMD_SEND_LCB_IDLE_MSG 0x04
#define WFR_HCMD_MISC		   0x05
#define WFR_HCMD_READ_LCB_IDLE_MSG 0x06
#define WFR_HCMD_INTERFACE_TEST	   0xff

/* DC_DC8051_CFG_HOST_CMD_1.RETURN_CODE - 8051 host command return */
#define WFR_HCMD_SUCCESS 2

/* DC_DC8051_DBG_ERR_INFO_SET_BY_8051.ERROR - error flags */
#define WFR_SPICO_ROM_FAILED		    (1 <<  0)
#define WFR_UNKNOWN_FRAME		    (1 <<  1)
#define WFR_TARGET_BER_NOT_MET		    (1 <<  2)
#define WFR_FAILED_SERDES_INTERNAL_LOOPBACK (1 <<  3)
#define WFR_FAILED_SERDES_INIT		    (1 <<  4)
#define WFR_FAILED_LNI_POLLING		    (1 <<  5)
#define WFR_FAILED_LNI_DEBOUNCE		    (1 <<  6)
#define WFR_FAILED_LNI_ESTBCOMM		    (1 <<  7)
#define WFR_FAILED_LNI_OPTEQ		    (1 <<  8)
#define WFR_FAILED_LNI_VERIFY_CAP1	    (1 <<  9)
#define WFR_FAILED_LNI_VERIFY_CAP2	    (1 << 10)
#define WFR_FAILED_LNI_CONFIGLT		    (1 << 11)

#define FAILED_LNI (WFR_FAILED_LNI_POLLING | WFR_FAILED_LNI_DEBOUNCE \
			| WFR_FAILED_LNI_ESTBCOMM | WFR_FAILED_LNI_OPTEQ \
			| WFR_FAILED_LNI_VERIFY_CAP1 \
			| WFR_FAILED_LNI_VERIFY_CAP2 \
			| WFR_FAILED_LNI_CONFIGLT)

/* DC_DC8051_DBG_ERR_INFO_SET_BY_8051.HOST_MSG - host message flags */
#define WFR_HOST_REQ_DONE	   (1 << 0)
#define WFR_BC_PWR_MGM_MSG	   (1 << 1)
#define WFR_BC_SMA_MSG		   (1 << 2)
#define WFR_BC_BCC_UNKOWN_MSG	   (1 << 3)
#define WFR_BC_IDLE_UNKNOWN_MSG	   (1 << 4)
#define WFR_EXT_DEVICE_CFG_REQ	   (1 << 5)
#define WFR_VERIFY_CAP_FRAME	   (1 << 6)
#define WFR_LINKUP_ACHIEVED	   (1 << 7)
#define WFR_LINK_GOING_DOWN	   (1 << 8)
#define WFR_LINK_WIDTH_DOWNGRADED  (1 << 9)

/* DC_DC8051_CFG_EXT_DEV_1.REQ_TYPE - 8051 host requests */
#define WFR_HREQ_LOAD_CONFIG	0x01
#define WFR_HREQ_SAVE_CONFIG	0x02
#define WFR_HREQ_READ_CONFIG	0x03
#define WFR_HREQ_SET_TX_EQ_ABS	0x04
#define WFR_HREQ_SET_TX_EQ_REL	0x05
#define WFR_HREQ_ENABLE		0x06
#define WFR_HREQ_CONFIG_DONE	0xfe
#define WFR_HREQ_INTERFACE_TEST	0xff

/* DC_DC8051_CFG_EXT_DEV_0.RETURN_CODE - 8051 host request return codes */
#define WFR_HREQ_INVALID		0x01
#define WFR_HREQ_SUCCESS		0x02
#define WFR_HREQ_NOT_SUPPORTED		0x03
#define WFR_HREQ_FEATURE_NOT_SUPPORTED	0x04 /* request specific feature */
#define WFR_HREQ_REQUEST_REJECTED	0xfe
#define WFR_HREQ_EXECUTION_ONGOING	0xff

/* MISC host command functions */
#define HCMD_MISC_REQUEST_LCB_ACCESS 0x1
#define HCMD_MISC_GRANT_LCB_ACCESS   0x2

/* idle flit message types */
#define WFR_IDLE_PHYSICAL_LINK_MGMT 0x1
#define WFR_IDLE_CRU		    0x2
#define WFR_IDLE_SMA		    0x3
#define WFR_IDLE_POWER_MGMT	    0x4

/* idle flit message send fields (both send and read) */
#define IDLE_PAYLOAD_MASK 0xffffffffffull /* 40 bits */
#define IDLE_PAYLOAD_SHIFT 8
#define IDLE_MSG_TYPE_MASK 0xf
#define IDLE_MSG_TYPE_SHIFT 0

/* idle flit message read fields */
#define READ_IDLE_MSG_TYPE_MASK 0xf
#define READ_IDLE_MSG_TYPE_SHIFT 0

/* SMA idle flit payload commands */
#define SMA_IDLE_ARM	1
#define SMA_IDLE_ACTIVE 2

/* DC_DC8051_CFG_MODE.GENERAL bits */
#define DISABLE_SELF_GUID_CHECK 0x2

/*
 * Eager buffer minimum and maximum sizes supported by the hardware.
 * All power-of-two sizes in between are supported as well.
 * WFR_MAX_EAGER_BUFFER_TOTAL is the maximum size of memory
 * allocatable for Eager buffer to a single context. All others
 * are limits for the RcvArray entries.
 */
#define WFR_MIN_EAGER_BUFFER       (4 * 1024)
#define WFR_MAX_EAGER_BUFFER       (256 * 1024)
#define WFR_MAX_EAGER_BUFFER_TOTAL (64 * (1 << 20)) /* max per ctxt 64MB */
#define WFR_MAX_EXPECTED_BUFFER    (2048 * 1024)

/*
 * Receive expected base and count and eager base and count increment -
 * the CSR fields hold multiples of this value.
 */
#define WFR_RCV_SHIFT 3
#define WFR_RCV_INCREMENT (1 << WFR_RCV_SHIFT)

/* CceDbiCtrl.Status values */
#define DBI_CTL_IDLE	0
#define DBI_CTL_BUSY	1
#define DBI_CTL_SUCCESS	2
#define DBI_CTL_ERROR	3

/*
 * Receive header queue entry increment - the CSR holds multiples of
 * this value.
 */
#define WFR_HDRQ_SIZE_SHIFT 5
#define WFR_HDRQ_INCREMENT (1 << WFR_HDRQ_SIZE_SHIFT)

/*
 * Freeze handling flags
 */
#define WFR_FREEZE_ABORT     0x01	/* do not do recovery */
#define WFR_FREEZE_SELF	     0x02	/* initiate the freeze */
#define WFR_FREEZE_LINK_DOWN 0x04	/* link is down */

/*
 * Chip implementation codes.
 */
#define WFR_ICODE_RTL_SILICON		0x00
#define WFR_ICODE_RTL_VCS_SIMULATION	0x01
#define WFR_ICODE_FPGA_EMULATION	0x02
#define WFR_ICODE_FUNCTIONAL_SIMULATOR	0x03

/* 8051 general register Field IDs */
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
#define LOAD_DATA_DATA_SHIFT   0x0
#define LOAD_DATA_DATA_MASK   0xffffffffull

/* READ_DATA 8051 command shifts and fields */
#define READ_DATA_FIELD_ID_SHIFT 40
#define READ_DATA_FIELD_ID_MASK 0xfull
#define READ_DATA_LANE_ID_SHIFT 32
#define READ_DATA_LANE_ID_MASK 0xfull
#define READ_DATA_DATA_SHIFT   0x0
#define READ_DATA_DATA_MASK   0xffffffffull

/* TX settings fields */
#define ENABLE_LANE_TX_SHIFT		0
#define ENABLE_LANE_TX_MASK		0xff
#define TX_POLARITY_INVERSION_SHIFT	8
#define TX_POLARITY_INVERSION_MASK	0xff
#define RX_POLARITY_INVERSION_SHIFT	16
#define RX_POLARITY_INVERSION_MASK	0xff
#define MAX_RATE_SHIFT			24
#define MAX_RATE_MASK			0xff

/* verify capibility PHY fields */
#define CONTINIOUS_REMOTE_UPDATE_SUPPORT_SHIFT	0x4
#define CONTINIOUS_REMOTE_UPDATE_SUPPORT_MASK	0x1
#define POWER_MANAGEMENT_SHIFT			0x0
#define POWER_MANAGEMENT_MASK			0xf

/* 8051 lane register Field IDs */
#define SPICO_FW_VERSION 0x7	/* SPICO firmware version */

/* SPICO firmware version fields */
#define SPICO_ROM_VERSION_SHIFT 0
#define SPICO_ROM_VERSION_MASK 0xffff
#define SPICO_ROM_PROD_ID_SHIFT 16
#define SPICO_ROM_PROD_ID_MASK 0xffff

/* verify capibility fabric fields */
#define VAU_SHIFT	0
#define VAU_MASK	0x0007
#define Z_SHIFT		3
#define Z_MASK		0x0001
#define VCU_SHIFT	4
#define VCU_MASK	0x0007
#define VL15BUF_SHIFT	8
#define VL15BUF_MASK	0x0fff
#define CRC_SIZES_SHIFT 20
#define CRC_SIZES_MASK	0x7

/* verify capability link width fields */
#define LINK_WIDTH_SHIFT 0
#define LINK_WIDTH_MASK 0xffff
#define LOCAL_FLAG_BITS_SHIFT 16
#define LOCAL_FLAG_BITS_MASK 0xff
#define REMOTE_TX_RATE_SHIFT 16
#define REMOTE_TX_RATE_MASK 0xff

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

/* local LNI link width fields */
#define ENABLE_LANE_RX_SHIFT 16
#define ENABLE_LANE_RX_MASK  0xff

/* mask, shift for reading 'mgmt_enabled' value from REMOTE_LNI_INFO field */
#define MGMT_ALLOWED_SHIFT 23
#define MGMT_ALLOWED_MASK 0x1

/* mask, shift for 'link_quality' within LINK_QUALITY_INFO field */
#define LINK_QUALITY_SHIFT 0
#define LINK_QUALITY_MASK  0x7

/*
 * mask, shift for reading 'planned_down_remote_reason_code'
 * from LINK_QUALITY_INFO field
 */
#define DOWN_REMOTE_REASON_SHIFT 16
#define DOWN_REMOTE_REASON_MASK  0xff

/* verify capability PHY power management bits */
#define PWRM_BER_CONTROL	0x1
#define PWRM_BANDWIDTH_CONTROL	0x2

/* verify capability fabric CRC size bits */
enum {
	CAP_CRC_14B = (1 << 0), /* 14b CRC */
	CAP_CRC_48B = (1 << 1), /* 48b CRC */
	CAP_CRC_12B_16B_PER_LANE = (1 << 2) /* 12b-16b per lane CRC */
};

#define WFR_SUPPORTED_CRCS (CAP_CRC_14B | CAP_CRC_48B | \
			    CAP_CRC_12B_16B_PER_LANE)

/* misc status version fields */
#define STS_FM_VERSION_A_SHIFT 16
#define STS_FM_VERSION_A_MASK  0xff
#define STS_FM_VERSION_B_SHIFT 24
#define STS_FM_VERSION_B_MASK  0xff

/* LCB_CFG_CRC_MODE TX_VAL and RX_VAL CRC mode values */
#define LCB_CRC_16B			0x0	/* 16b CRC */
#define LCB_CRC_14B			0x1	/* 14b CRC */
#define LCB_CRC_48B			0x2	/* 48b CRC */
#define LCB_CRC_12B_16B_PER_LANE	0x3	/* 12b-16b per lane CRC */

/* the following enum is (almost) a copy/paste of the definition
 * in the STL spec, section 20.2.2.6.8 (PortInfo) */
enum {
	PORT_LTP_CRC_MODE_NONE = 0,
	PORT_LTP_CRC_MODE_14 = 1, /* 14-bit LTP CRC mode (optional) */
	PORT_LTP_CRC_MODE_16 = 2, /* 16-bit LTP CRC mode */
	PORT_LTP_CRC_MODE_48 = 4,
		/* 48-bit overlapping LTP CRC mode (optional) */
	PORT_LTP_CRC_MODE_PER_LANE = 8
		/* 12 to 16 bit per lane LTP CRC mode (optional) */
};

/* timeouts */
#define LINK_RESTART_DELAY 1000		/* link restart delay, in ms */
#define DC8051_COMMAND_TIMEOUT 5000	/* DC8051 command timeout, in ms */
#define FREEZE_STATUS_TIMEOUT 20	/* wait for freeze indicators, in ms */
#define DBI_TIMEOUT 1			/* DBI hardare access timeout, in ms */
#define VL_STATUS_CLEAR_TIMEOUT 5000	/* per-VL status clear, in ms */
#define CCE_STATUS_TIMEOUT 10		/* time to clear CCE Status, in ms */

/* cclock tick time, in picoseconds per tick: 1/speed * 10^12  */
#define ASIC_CCLOCK_PS  1242	/* 805 MHz */
#define FPGA_CCLOCK_PS 30300	/*  33 MHz */

/*
 * Mask of enabled MISC errors.  Do not enable the two RSA engine errors -
 * see firmware.c:run_rsa() for details.
 */
#define DRIVER_MISC_MASK \
	(~(WFR_MISC_ERR_STATUS_MISC_FW_AUTH_FAILED_ERR_SMASK \
		| WFR_MISC_ERR_STATUS_MISC_KEY_MISMATCH_ERR_SMASK))

/* valid values for the loopback module parameter */
#define LOOPBACK_NONE	0	/* no loopback - default */
#define LOOPBACK_SERDES 1
#define LOOPBACK_LCB	2
#define LOOPBACK_CABLE	3	/* external cable */

/* read and write hardware registers */
u64 read_csr(const struct hfi_devdata *dd, u32 offset);
void write_csr(const struct hfi_devdata *dd, u32 offset, u64 value);

/*
 * The *_kctxt_* flavor of the CSR read/write functions are for
 * per-context or per-SDMA CSRs that are not mappable to user-space.
 * Their spacing is not a PAGE_SIZE multiple.
 */
static inline u64 read_kctxt_csr(const struct hfi_devdata *dd, int ctxt,
					u32 offset0)
{
	/* kernel per-context CSRs are separated by 0x100 */
	return read_csr(dd, offset0 + (0x100 * ctxt));
}

static inline void write_kctxt_csr(struct hfi_devdata *dd, int ctxt,
					u32 offset0, u64 value)
{
	/* kernel per-context CSRs are separated by 0x100 */
	write_csr(dd, offset0 + (0x100 * ctxt), value);
}

void __iomem *get_csr_addr(
	struct hfi_devdata *dd,
	u32 offset);

static inline void __iomem *get_kctxt_csr_addr(
	struct hfi_devdata *dd,
	int ctxt,
	u32 offset0)
{
	return get_csr_addr(dd, offset0 + (0x100 * ctxt));
}

/*
 * The *_uctxt_* flavor of the CSR read/write functions are for
 * per-context CSRs that are mappable to user space. All these CSRs
 * are spaced by a PAGE_SIZE multiple in order to be mappable to
 * different processes without exposing other contexts' CSRs
 */
static inline u64 read_uctxt_csr(const struct hfi_devdata *dd, int ctxt,
				 u32 offset0)
{
	/* user per-context CSRs are separated by 0x1000 */
	return read_csr(dd, offset0 + (0x1000 * ctxt));
}

static inline void write_uctxt_csr(struct hfi_devdata *dd, int ctxt,
				   u32 offset0, u64 value)
{
	/* TODO: write to user mapping if available? */
	/* user per-context CSRs are separated by 0x1000 */
	write_csr(dd, offset0 + (0x1000 * ctxt), value);
}

u64 create_pbc(u64, u32, u32, u32);

/* firmware.c */
#define NUM_PCIE_SERDES 16	/* number of PCIe serdes on the SBus */
extern const u8 pcie_serdes_broadcast[];
extern const u8 pcie_pcs_addrs[2][NUM_PCIE_SERDES];
/* SBUS commands */
#define RESET_SBUS_RECEIVER 0x20
#define WRITE_SBUS_RECEIVER 0x21
#define READ_SBUS_RECEIVER  0x22
void sbus_request(struct hfi_devdata *dd,
		  u8 receiver_addr, u8 data_addr, u8 command, u32 data_in);
int sbus_request_slow(struct hfi_devdata *dd,
		      u8 receiver_addr, u8 data_addr, u8 command, u32 data_in);
void set_sbus_fast_mode(struct hfi_devdata *dd);
void clear_sbus_fast_mode(struct hfi_devdata *dd);
int firmware_init(struct hfi_devdata *dd);
int load_pcie_firmware(struct hfi_devdata *dd);
int load_firmware(struct hfi_devdata *dd);
void dispose_firmware(void);
int acquire_hw_mutex(struct hfi_devdata *dd);
void release_hw_mutex(struct hfi_devdata *dd);
void fabric_serdes_reset(struct hfi_devdata *dd);

/* wfr.c */
void read_misc_status(struct hfi_devdata *dd, u8 *ver_a, u8 *ver_b);
void read_guid(struct hfi_devdata *dd);
int wait_fm_ready(struct hfi_devdata *dd, u32 mstimeout);
void check_fifos(unsigned long opaque);
void set_link_down_reason(struct qib_pportdata *ppd, u8 lcl_reason,
	u8 neigh_reason, u8 rem_reason);
int set_link_state(struct qib_pportdata *, u32 state);
int port_ltp_to_cap(int port_ltp);
void handle_verify_cap(struct work_struct *work);
void handle_freeze(struct work_struct *work);
void handle_link_up(struct work_struct *work);
void handle_link_down(struct work_struct *work);
void handle_link_downgrade(struct work_struct *work);
void handle_sma_message(struct work_struct *work);
void start_freeze_handling(struct qib_pportdata *ppd, int flags);
int send_idle_sma(struct hfi_devdata *dd, u64 message);
int start_link(struct qib_pportdata *ppd);
void init_qsfp(struct qib_pportdata *ppd);
int bringup_serdes(struct qib_pportdata *ppd);
void set_intr_state(struct hfi_devdata *dd, u32 enable);
void apply_link_downgrade_policy(struct qib_pportdata *ppd, int refresh_widths);
void update_usrhead(struct qib_ctxtdata *, u32, u32, u32, u32, u32);
int stop_drain_data_vls(struct hfi_devdata *dd);
int open_fill_data_vls(struct hfi_devdata *dd);
u32 ns_to_cclock(struct hfi_devdata *dd, u32 ns);
u32 cclock_to_ns(struct hfi_devdata *dd, u32 cclock);
void get_linkup_link_widths(struct qib_pportdata *ppd);
void read_ltp_rtt(struct hfi_devdata *dd);
void clear_linkup_counters(struct hfi_devdata *dd);
u32 hdrqempty(struct qib_ctxtdata *rcd);
int is_a0(struct hfi_devdata *dd);
int is_ax(struct hfi_devdata *dd);
int is_bx(struct hfi_devdata *dd);
void force_all_interrupts(struct hfi_devdata *dd);

int acquire_lcb_access(struct hfi_devdata *dd, int sleep_ok);
int release_lcb_access(struct hfi_devdata *dd, int sleep_ok);
#define LCB_START DC_LCB_CSRS
#define LCB_END   DC_8051_CSRS /* next block is 8051 */
static inline int is_lcb_offset(u32 offset)
{
	return (offset >= LCB_START && offset < LCB_END);
}

extern uint num_vls;

extern uint disable_integrity;
u64 read_dev_cntr(struct hfi_devdata *dd, int index, int vl);
u64 write_dev_cntr(struct hfi_devdata *dd, int index, int vl, u64 data);
u64 read_port_cntr(struct qib_pportdata *ppd, int index, int vl);
u64 write_port_cntr(struct qib_pportdata *ppd, int index, int vl, u64 data);

/* Per VL indexes */
enum {
	C_VL_0 = 0,
	C_VL_1,
	C_VL_2,
	C_VL_3,
	C_VL_4,
	C_VL_5,
	C_VL_6,
	C_VL_7,
	C_VL_15,
	C_VL_COUNT
};

static inline int vl_from_idx(int idx)
{
	return (idx == C_VL_15 ? 15 : idx);
}

static inline int idx_from_vl(int vl)
{
	return (vl == 15 ? C_VL_15 : vl);
}

/* Per device counter indexes */
enum {
	C_RCV_OVF = 0,
	C_RX_TID_FULL,
	C_RX_TID_INVALID,
	C_RX_TID_FLGMS,
	C_RX_CTX_RHQS,
	C_RX_CTX_EGRS,
	C_RCV_TID_FLSMS,
	C_CCE_PCI_CR_ST,
	C_CCE_PCI_TR_ST,
	C_CCE_PIO_WR_ST,
	C_CCE_ERR_INT,
	C_CCE_SDMA_INT,
	C_CCE_MISC_INT,
	C_CCE_RCV_AV_INT,
	C_CCE_RCV_URG_INT,
	C_CCE_SEND_CR_INT,
	C_DC_UNC_ERR,
	C_DC_RCV_ERR,
	C_DC_FM_CFG_ERR,
	C_DC_RMT_PHY_ERR,
	C_DC_DROPPED_PKT,
	C_DC_MC_XMIT_PKTS,
	C_DC_MC_RCV_PKTS,
	C_DC_XMIT_CERR,
	C_DC_RCV_CERR,
	C_DC_RCV_FCC,
	C_DC_XMIT_FCC,
	C_DC_XMIT_FLITS,
	C_DC_RCV_FLITS,
	C_DC_XMIT_PKTS,
	C_DC_RCV_PKTS,
	C_DC_RX_FLIT_VL,
	C_DC_RX_PKT_VL,
	C_DC_RCV_FCN,
	C_DC_RCV_FCN_VL,
	C_DC_RCV_BCN,
	C_DC_RCV_BCN_VL,
	C_DC_RCV_BBL,
	C_DC_RCV_BBL_VL,
	C_DC_MARK_FECN,
	C_DC_MARK_FECN_VL,
	C_DC_TOTAL_CRC,
	C_DC_CRC_LN0,
	C_DC_CRC_LN1,
	C_DC_CRC_LN2,
	C_DC_CRC_LN3,
	C_DC_CRC_MULT_LN,
	C_DC_TX_REPLAY,
	C_DC_RX_REPLAY,
	C_DC_SEQ_CRC_CNT,
	C_DC_ESC0_ONLY_CNT,
	C_DC_ESC0_PLUS1_CNT,
	C_DC_ESC0_PLUS2_CNT,
	C_DC_REINIT_FROM_PEER_CNT,
	C_DC_SBE_CNT,
	C_DC_MISC_FLG_CNT,
	C_DC_PRF_GOOD_LTP_CNT,
	C_DC_PRF_ACCEPTED_LTP_CNT,
	C_DC_PRF_RX_FLIT_CNT,
	C_DC_PRF_TX_FLIT_CNT,
	C_DC_PRF_CLK_CNTR,
	C_DC_PG_DBG_FLIT_CRDTS_CNT,
	C_DC_PG_STS_PAUSE_COMPLETE_CNT,
	C_DC_PG_STS_TX_SBE_CNT,
	C_DC_PG_STS_TX_MBE_CNT,
	C_SW_CPU_INTR,
	DEV_CNTR_LAST  /* Must be kept last */
};

/* Per port counter indexes */
enum {
	C_TX_UNSUP_VL = 0,
	C_TX_INVAL_LEN,
	C_TX_MM_LEN_ERR,
	C_TX_UNDERRUN,
	C_TX_FLOW_STALL,
	C_TX_DROPPED,
	C_TX_HDR_ERR,
	C_TX_PKT,
	C_TX_WORDS,
	C_TX_WAIT,
	C_TX_FLIT_VL,
	C_TX_PKT_VL,
	C_TX_WAIT_VL,
	C_RX_PKT,
	C_RX_WORDS,
	C_SW_LINK_DOWN,
	C_SW_LINK_UP,
	C_SW_XMIT_DSCD,
	C_SW_XMIT_DSCD_VL,
	C_SW_XMIT_CSTR_ERR,
	C_SW_RCV_CSTR_ERR,
	C_SW_IBP_LOOP_PKTS,
	C_SW_IBP_RC_RESENDS,
	C_SW_IBP_RNR_NAKS,
	C_SW_IBP_OTHER_NAKS,
	C_SW_IBP_RC_TIMEOUTS,
	C_SW_IBP_PKT_DROPS,
	C_SW_IBP_DMA_WAIT,
	C_SW_IBP_RC_SEQNAK,
	C_SW_IBP_RC_DELAYED_COMP,
	C_SW_IBP_RC_DUPREQ,
	C_SW_IBP_RDMA_SEQ,
	C_SW_IBP_UNALIGNED,
	C_SW_IBP_SEQ_NAK,
	C_SW_CPU_RC_ACKS,
	C_SW_CPU_RC_QACKS,
	C_RCV_HDR_OVF_0,
	C_RCV_HDR_OVF_1,
	C_RCV_HDR_OVF_2,
	C_RCV_HDR_OVF_3,
	C_RCV_HDR_OVF_4,
	C_RCV_HDR_OVF_5,
	C_RCV_HDR_OVF_6,
	C_RCV_HDR_OVF_7,
	C_RCV_HDR_OVF_8,
	C_RCV_HDR_OVF_9,
	C_RCV_HDR_OVF_10,
	C_RCV_HDR_OVF_11,
	C_RCV_HDR_OVF_12,
	C_RCV_HDR_OVF_13,
	C_RCV_HDR_OVF_14,
	C_RCV_HDR_OVF_15,
	C_RCV_HDR_OVF_16,
	C_RCV_HDR_OVF_17,
	C_RCV_HDR_OVF_18,
	C_RCV_HDR_OVF_19,
	C_RCV_HDR_OVF_20,
	C_RCV_HDR_OVF_21,
	C_RCV_HDR_OVF_22,
	C_RCV_HDR_OVF_23,
	C_RCV_HDR_OVF_24,
	C_RCV_HDR_OVF_25,
	C_RCV_HDR_OVF_26,
	C_RCV_HDR_OVF_27,
	C_RCV_HDR_OVF_28,
	C_RCV_HDR_OVF_29,
	C_RCV_HDR_OVF_30,
	C_RCV_HDR_OVF_31,
	C_RCV_HDR_OVF_32,
	C_RCV_HDR_OVF_33,
	C_RCV_HDR_OVF_34,
	C_RCV_HDR_OVF_35,
	C_RCV_HDR_OVF_36,
	C_RCV_HDR_OVF_37,
	C_RCV_HDR_OVF_38,
	C_RCV_HDR_OVF_39,
	C_RCV_HDR_OVF_40,
	C_RCV_HDR_OVF_41,
	C_RCV_HDR_OVF_42,
	C_RCV_HDR_OVF_43,
	C_RCV_HDR_OVF_44,
	C_RCV_HDR_OVF_45,
	C_RCV_HDR_OVF_46,
	C_RCV_HDR_OVF_47,
	C_RCV_HDR_OVF_48,
	C_RCV_HDR_OVF_49,
	C_RCV_HDR_OVF_50,
	C_RCV_HDR_OVF_51,
	C_RCV_HDR_OVF_52,
	C_RCV_HDR_OVF_53,
	C_RCV_HDR_OVF_54,
	C_RCV_HDR_OVF_55,
	C_RCV_HDR_OVF_56,
	C_RCV_HDR_OVF_57,
	C_RCV_HDR_OVF_58,
	C_RCV_HDR_OVF_59,
	C_RCV_HDR_OVF_60,
	C_RCV_HDR_OVF_61,
	C_RCV_HDR_OVF_62,
	C_RCV_HDR_OVF_63,
	C_RCV_HDR_OVF_64,
	C_RCV_HDR_OVF_65,
	C_RCV_HDR_OVF_66,
	C_RCV_HDR_OVF_67,
	C_RCV_HDR_OVF_68,
	C_RCV_HDR_OVF_69,
	C_RCV_HDR_OVF_70,
	C_RCV_HDR_OVF_71,
	C_RCV_HDR_OVF_72,
	C_RCV_HDR_OVF_73,
	C_RCV_HDR_OVF_74,
	C_RCV_HDR_OVF_75,
	C_RCV_HDR_OVF_76,
	C_RCV_HDR_OVF_77,
	C_RCV_HDR_OVF_78,
	C_RCV_HDR_OVF_79,
	C_RCV_HDR_OVF_80,
	C_RCV_HDR_OVF_81,
	C_RCV_HDR_OVF_82,
	C_RCV_HDR_OVF_83,
	C_RCV_HDR_OVF_84,
	C_RCV_HDR_OVF_85,
	C_RCV_HDR_OVF_86,
	C_RCV_HDR_OVF_87,
	C_RCV_HDR_OVF_88,
	C_RCV_HDR_OVF_89,
	C_RCV_HDR_OVF_90,
	C_RCV_HDR_OVF_91,
	C_RCV_HDR_OVF_92,
	C_RCV_HDR_OVF_93,
	C_RCV_HDR_OVF_94,
	C_RCV_HDR_OVF_95,
	C_RCV_HDR_OVF_96,
	C_RCV_HDR_OVF_97,
	C_RCV_HDR_OVF_98,
	C_RCV_HDR_OVF_99,
	C_RCV_HDR_OVF_100,
	C_RCV_HDR_OVF_101,
	C_RCV_HDR_OVF_102,
	C_RCV_HDR_OVF_103,
	C_RCV_HDR_OVF_104,
	C_RCV_HDR_OVF_105,
	C_RCV_HDR_OVF_106,
	C_RCV_HDR_OVF_107,
	C_RCV_HDR_OVF_108,
	C_RCV_HDR_OVF_109,
	C_RCV_HDR_OVF_110,
	C_RCV_HDR_OVF_111,
	C_RCV_HDR_OVF_112,
	C_RCV_HDR_OVF_113,
	C_RCV_HDR_OVF_114,
	C_RCV_HDR_OVF_115,
	C_RCV_HDR_OVF_116,
	C_RCV_HDR_OVF_117,
	C_RCV_HDR_OVF_118,
	C_RCV_HDR_OVF_119,
	C_RCV_HDR_OVF_120,
	C_RCV_HDR_OVF_121,
	C_RCV_HDR_OVF_122,
	C_RCV_HDR_OVF_123,
	C_RCV_HDR_OVF_124,
	C_RCV_HDR_OVF_125,
	C_RCV_HDR_OVF_126,
	C_RCV_HDR_OVF_127,
	C_RCV_HDR_OVF_128,
	C_RCV_HDR_OVF_129,
	C_RCV_HDR_OVF_130,
	C_RCV_HDR_OVF_131,
	C_RCV_HDR_OVF_132,
	C_RCV_HDR_OVF_133,
	C_RCV_HDR_OVF_134,
	C_RCV_HDR_OVF_135,
	C_RCV_HDR_OVF_136,
	C_RCV_HDR_OVF_137,
	C_RCV_HDR_OVF_138,
	C_RCV_HDR_OVF_139,
	C_RCV_HDR_OVF_140,
	C_RCV_HDR_OVF_141,
	C_RCV_HDR_OVF_142,
	C_RCV_HDR_OVF_143,
	C_RCV_HDR_OVF_144,
	C_RCV_HDR_OVF_145,
	C_RCV_HDR_OVF_146,
	C_RCV_HDR_OVF_147,
	C_RCV_HDR_OVF_148,
	C_RCV_HDR_OVF_149,
	C_RCV_HDR_OVF_150,
	C_RCV_HDR_OVF_151,
	C_RCV_HDR_OVF_152,
	C_RCV_HDR_OVF_153,
	C_RCV_HDR_OVF_154,
	C_RCV_HDR_OVF_155,
	C_RCV_HDR_OVF_156,
	C_RCV_HDR_OVF_157,
	C_RCV_HDR_OVF_158,
	C_RCV_HDR_OVF_159,
	PORT_CNTR_LAST /* Must be kept last */
};

/*
 * FIXME: These are non-exported register definitions that are still
 * used in the driver.  Remove when their use is removed.
 */
#define WFR_CCE_DBI_CTRL                         (WFR_CCE + 0x0000000000A0)
#define WFR_CCE_DBI_CTRL_RESETCSR                0x0000000000000000ull
#define WFR_CCE_DBI_CTRL_STATUS_SHIFT            6
#define WFR_CCE_DBI_CTRL_STATUS_MASK             0x3ull
#define WFR_CCE_DBI_CTRL_STATUS_SMASK            0xC0ull
#define WFR_CCE_DBI_CTRL_CS_SHIFT                4
#define WFR_CCE_DBI_CTRL_CS_MASK                 0x3ull
#define WFR_CCE_DBI_CTRL_CS_SMASK                0x30ull
#define WFR_CCE_DBI_CTRL_WRITE_ENABLES_SHIFT     0
#define WFR_CCE_DBI_CTRL_WRITE_ENABLES_MASK      0xFull
#define WFR_CCE_DBI_CTRL_WRITE_ENABLES_SMASK     0xFull

#define WFR_CCE_DBI_ADDR                         (WFR_CCE + 0x0000000000A8)
#define WFR_CCE_DBI_ADDR_RESETCSR                0x0000000000000000ull
#define WFR_CCE_DBI_ADDR_ADDR_SHIFT              0
#define WFR_CCE_DBI_ADDR_ADDR_MASK               0xFFFFFFFFull
#define WFR_CCE_DBI_ADDR_ADDR_SMASK              0xFFFFFFFFull

#define WFR_CCE_DBI_DATA                         (WFR_CCE + 0x0000000000B0)
#define WFR_CCE_DBI_DATA_RESETCSR                0x0000000000000000ull
#define WFR_CCE_DBI_DATA_DATA_SHIFT              0
#define WFR_CCE_DBI_DATA_DATA_MASK               0xFFFFFFFFull
#define WFR_CCE_DBI_DATA_DATA_SMASK              0xFFFFFFFFull

#define WFR_ASIC_WFR_EFUSE_REGS6                (WFR_ASIC + 0x000000001430)
#define WFR_ASIC_WFR_EFUSE_REGS6_RESETCSR       0x0000000000000000ull
#define WFR_ASIC_WFR_EFUSE_REGS6_EFUSE_PATCH_VERSION_SHIFT 32
#define WFR_ASIC_WFR_EFUSE_REGS6_EFUSE_PATCH_VERSION_MASK 0xFFFFFFFFull
#define WFR_ASIC_WFR_EFUSE_REGS6_EFUSE_PATCH_VERSION_SMASK 0xFFFFFFFF00000000ull
#define WFR_ASIC_WFR_EFUSE_REGS6_EFUSE_DC_HFI1_SHIFT 24
#define WFR_ASIC_WFR_EFUSE_REGS6_EFUSE_DC_HFI1_MASK 0xFFull
#define WFR_ASIC_WFR_EFUSE_REGS6_EFUSE_DC_HFI1_SMASK 0xFF000000ull
#define WFR_ASIC_WFR_EFUSE_REGS6_EFUSE_DC_HFI0_SHIFT 16
#define WFR_ASIC_WFR_EFUSE_REGS6_EFUSE_DC_HFI0_MASK 0xFFull
#define WFR_ASIC_WFR_EFUSE_REGS6_EFUSE_DC_HFI0_SMASK 0xFF0000ull
#define WFR_ASIC_WFR_EFUSE_REGS6_EFUSE_VERSION_SHIFT 0
#define WFR_ASIC_WFR_EFUSE_REGS6_EFUSE_VERSION_MASK 0xFFFFull
#define WFR_ASIC_WFR_EFUSE_REGS6_EFUSE_VERSION_SMASK 0xFFFFull
/* FIXME: End of CSR defines to be removed */

u64 get_all_cpu_total(u64 __percpu *cntr);
#endif /* _WFR_H */

