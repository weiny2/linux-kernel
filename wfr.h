#ifndef _WFR_H
#define _WFR_H
/*
 * Copyright (c) 2013 Intel Corporation.  All rights reserved.
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

#ifdef CONFIG_STL_MGMT
#include "rdma/stl_smi.h"
#include "rdma/stl_port_info.h"
#endif

/* not defined in wfr_core.h */
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
#define WFR_PBC_STATIC_RATE_CONTROL_COUNT_SMASK (WFR_PBC_STATIC_RATE_CONTROL_COUNT_MASK << WFR_PBC_STATIC_RATE_CONTROL_COUNT_SHIFT)

#define WFR_PBC_INSERT_HCRC_SHIFT 26
#define WFR_PBC_INSERT_HCRC_MASK 0x3ull
#define WFR_PBC_INSERT_HCRC_SMASK (WFR_PBC_INSERT_HCRC_MASK << WFR_PBC_INSERT_HCRC_SHIFT)

#define WFR_PBC_VL_SHIFT 12
#define WFR_PBC_VL_MASK 0xfull
#define WFR_PBC_VL_SMASK (WFR_PBC_VL_MASK << WFR_PBC_VL_SHIFT)

#define WFR_PBC_LENGTH_DWS_SHIFT 0
#define WFR_PBC_LENGTH_DWS_MASK 0xfffull
#define WFR_PBC_LENGTH_DWS_SMASK (WFR_PBC_LENGTH_DWS_MASK << WFR_PBC_LENGTH_DWS_SHIFT)

/* Credit Return Fields */
#define WFR_CR_COUNTER_SHIFT 0
#define WFR_CR_COUNTER_MASK 0x7ffull
#define WFR_CR_COUNTER_SMASK (WFR_CR_COUNTER_MASK << WFR_CR_COUNTER_SHIFT)

#define WFR_CR_STATUS_SHIFT 11
#define WFR_CR_STATUS_MASK 0x1ull
#define WFR_CR_STATUS_SMASK (WFR_CR_STATUS_MASK << WFR_CR_STATUS_SHIFT)

#define WFR_CR_CREDIT_RETURN_DUE_TO_PBC_SHIFT 12
#define WFR_CR_CREDIT_RETURN_DUE_TO_PBC_MASK 0x1ull
#define WFR_CR_CREDIT_RETURN_DUE_TO_PBC_SMASK (WFR_CR_CREDIT_RETURN_DUE_TO_PBC_MASK << WFR_CR_CREDIT_RETURN_DUE_TO_PBC_SHIFT)

#define WFR_CR_CREDIT_RETURN_DUE_TO_THRESHOLD_SHIFT 13
#define WFR_CR_CREDIT_RETURN_DUE_TO_THRESHOLD_MASK 0x1ull
#define WFR_CR_CREDIT_RETURN_DUE_TO_THRESHOLD_SMASK (WFR_CR_CREDIT_RETURN_DUE_TO_THRESHOLD_MASK << WFR_CR_CREDIT_RETURN_DUE_TO_THRESHOLD_SHIFT)

#define WFR_CR_CREDIT_RETURN_DUE_TO_ERR_SHIFT 14
#define WFR_CR_CREDIT_RETURN_DUE_TO_ERR_MASK 0x1ull
#define WFR_CR_CREDIT_RETURN_DUE_TO_ERR_SMASK (WFR_CR_CREDIT_RETURN_DUE_TO_ERR_MASK << WFR_CR_CREDIT_RETURN_DUE_TO_ERR_SHIFT)

#define WFR_CR_CREDIT_RETURN_DUE_TO_FORCE_SHIFT 15
#define WFR_CR_CREDIT_RETURN_DUE_TO_FORCE_MASK 0x1ull
#define WFR_CR_CREDIT_RETURN_DUE_TO_FORCE_SMASK (WFR_CR_CREDIT_RETURN_DUE_TO_FORCE_MASK << WFR_CR_CREDIT_RETURN_DUE_TO_FORCE_SHIFT)

/* interrupt source numbers */
#define WFR_IS_GENERAL_ERR_START	  0
#define WFR_IS_SDMAENG_ERR_START	 16
#define WFR_IS_SENDCTXT_ERR_START	 32
#define WFR_IS_SDMA_START	 	192 /* includes SDmaProgress,SDmaIdle */
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
#define WFR_HCMD_LOAD_CONFIG_DATA 0x01
#define WFR_HCMD_READ_CONFIG_DATA 0x02
#define WFR_HCMD_CHANGE_PHY_STATE 0x03
#define WFR_HCMD_SEND_BCC_MSG	  0x04
#define WFR_HCMD_MISC		  0x05
#define WFR_HCMD_INTERFACE_TEST	  0xff

/* DC_DC8051_CFG_HOST_CMD_1.RETURN_CODE - 8051 host command return */
#define WFR_HCMD_SUCCESS 2

/* DC_DC8051_DBG_ERR_INFO_SET_BY_8051.ERROR - error flags */
#define WFR_SPICO_ROM_FAIL 0x01
#define WFR_UNKOWN_FRAME   0x02
#define WFR_BER_NOT_MET	   0x04
#define WFR_LOOPBACK_FAIL  0x08

/* DC_DC8051_DBG_ERR_INFO_SET_BY_8051.HOST_MSG - host message flags */
#define WFR_HOST_REQ_DONE	   0x0001
#define WFR_BC_LCB_IDLE_FRAME_MSG  0x0002
#define WFR_BC_BCC_FRAME_MSG	   0x0004
#define WFR_BC_LCB_IDLE_UNKOWN_MSG 0x0008
#define WFR_BC_BCC_UNKNOWN_MSG	   0x0010
#define WFR_EXT_DEVICE_CFG_REQ	   0x0020
#define WFR_VERIFY_CAP_FRAME	   0x0040
#define WFR_LINKUP_ACHIEVED	   0x0080
#define WFR_LINK_GOING_DOWN	   0x0100

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

/*
 * Eager buffer minimum and maximum sizes supported by the hardware.
 * All power-of-two sizes in between are supported as well.
 */
#define WFR_MIN_EAGER_BUFFER (  4 * 1024)
#define WFR_MAX_EAGER_BUFFER (256 * 1024)

/*
 * Receive expected base and count and eager base and count increment -
 * the CSR fields hold multiples of this value.
 */
#define WFR_RCV_SHIFT 3
#define WFR_RCV_INCREMENT (1 << WFR_RCV_SHIFT)

/*
 * Receive header queue entry increment - the CSR holds multiples of
 * this value.
 */
#define WFR_HDRQ_SIZE_SHIFT 5
#define WFR_HDRQ_INCREMENT (1 << WFR_HDRQ_SIZE_SHIFT)

/*
 * Chip implementation codes.
 */
#define WFR_ICODE_RTL_SILICON		0x00
#define WFR_ICODE_RTL_VCS_SIMULATION	0x01
#define WFR_ICODE_FPGA_EMULATION	0x02
#define WFR_ICODE_FUNCTIONAL_SIMULATOR	0x03

/* 8051 general register Field IDs */
#define VERIFY_CAP_LOCAL_PHY	     0x07
#define VERIFY_CAP_LOCAL_FABRIC	     0x08
#define VERIFY_CAP_LOCAL_LINK_WIDTH  0x09
#define VERIFY_CAP_REMOTE_PHY	     0x0f
#define VERIFY_CAP_REMOTE_FABRIC     0x10
#define VERIFY_CAP_REMOTE_LINK_WIDTH 0x11

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

/* verify capibility PHY fields */
#define CONTINIOUS_REMOTE_UPDATE_SUPPORT_SHIFT	0x4
#define CONTINIOUS_REMOTE_UPDATE_SUPPORT_MASK	0x1
#define POWER_MANAGEMENT_SHIFT			0x0
#define POWER_MANAGEMENT_MASK			0xf

/* verify capibility fabric fields */
#define VAU_SHIFT 0x0
#define VAU_MASK 0xf
#define VCU_SHIFT 0x4
#define VCU_MASK 0xf
#define VL15BUF_SHIFT 8
#define VL15BUF_MASK 0xfff
#define CRC_SIZES_SHIFT 20
#define CRC_SIZES_MASK 0x7

/* verify capability link width fields */
#define LINK_WIDTH_SHIFT 0
#define LINK_WIDTH_MASK 0xffff
#define FLAG_BITS_SHIFT 16
#define FLAG_BITS_MASK 0xffff

/* verify capability link width values */
#define WFR_SUPPORTED_LINK_WIDTHS 0xb	/* 4,2,1 widths */

/* verify capability PHY power management bits */
#define PWRM_BER_CONTROL	0x1
#define PWRM_BANDWIDTH_CONTROL	0x2
#define PWRM_SHALLOW_SLEEP	0x4
#define PWRM_DEEP_SLEEP		0x8

/* verify capability fabric CRC size bits */
#define CRC_14B			0x1	/* 14b CRC */
#define CRC_48B			0x2	/* 48b CRC */
#define CRC_12B_16B_PER_LANE	0x4	/* 12b-16b per lane CRC */

#define WFR_SUPPORTED_CRCS (CRC_14B | CRC_48B | CRC_12B_16B_PER_LANE)

/* LCB_CFG_CRC_MODE TX_VAL and RX_VAL CRC mode values */
#define LCB_CRC_16B			0x0	/* 16b CRC */
#define LCB_CRC_14B			0x1	/* 14b CRC */
#define LCB_CRC_48B			0x2	/* 48b CRC */
#define LCB_CRC_12B_16B_PER_LANE	0x3	/* 12b-16b per lane CRC */

/* read and write hardware registers */
u64 read_csr(const struct hfi_devdata *dd, u32 offset);
void write_csr(const struct hfi_devdata *dd, u32 offset, u64 value);

/*
 *
 * The *_kctxt_* flavor of the CSR read/write functions are for
 * per-context or per-SDMA CSRs that are not mappable to user-space.
 * Their spacing is not a PAGE_SIZE multiple.
 */
static inline u64 read_kctxt_csr(const struct hfi_devdata *dd, int ctxt,
					u32 offset0)
{
	/* kernel per-context CSRs are separated by 0x100 */
	return read_csr(dd, offset0 + (0x100* ctxt));
}

static inline void write_kctxt_csr(struct hfi_devdata *dd, int ctxt,
					u32 offset0, u64 value)
{
	/* kernel per-context CSRs are separated by 0x100 */
	write_csr(dd, offset0 + (0x100 * ctxt), value);
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

static inline int stl_speed_to_ib(u16 in)
{
#ifdef CONFIG_STL_MGMT
	if (in & STL_LINK_SPEED_25G) {
		in &= ~STL_LINK_SPEED_25G;
		in |= IB_SPEED_EDR;
	}

	if (in & STL_LINK_SPEED_12_5G) {
		in &= ~STL_LINK_SPEED_12_5G;
		in |= IB_SPEED_QDR;
	}
#endif
	BUG_ON(!in);
	return in;
}

static inline int stl_width_to_ib(u16 in)
{
#ifdef CONFIG_STL_MGMT
	in &= ~(STL_LINK_WIDTH_2X | STL_LINK_WIDTH_3X);
#endif
	BUG_ON(!in);
	return in;
}

int load_firmware(struct hfi_devdata *dd);
void dispose_firmware(void);
void read_guid(struct hfi_devdata *dd);
void check_fifos(unsigned long opaque);

extern uint num_vls;

#endif /* _WFR_H */

