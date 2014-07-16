/*
 * Copyright (c) 2012 Intel Corporation.  All rights reserved.
 * Copyright (c) 2008 - 2012 QLogic Corporation. All rights reserved.
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
 * This file contains all of the code that is specific to the WFR chip
 */

#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/module.h>

#include "hfi.h"
#include "trace.h"
#include "mad.h"
#include "pio.h"

#define NUM_IB_PORTS 1

/* JAG SDMA from HAS 7.3.1 */
#define DEFAULT_WFR_SEND_DMA_MEMORY (((unsigned long long) \
	(TXE_SDMA_MEMORY_BYTES/(TXE_NUM_SDMA_ENGINES * 64))) \
	<< WFR_SEND_DMA_MEMORY_SDMA_MEMORY_CNT_SHIFT )

static uint mod_num_sdma;
module_param_named(num_sdma, mod_num_sdma, uint, S_IRUGO);
MODULE_PARM_DESC(num_sdma, "Set max number SDMA engines to use");

uint kdeth_qp;
module_param_named(kdeth_qp, kdeth_qp, uint, S_IRUGO);
MODULE_PARM_DESC(kdeth_qp, "Set the KDETH queue pair prefix");

uint num_vls = 4;
module_param(num_vls, uint, S_IRUGO);
MODULE_PARM_DESC(num_vls, "Set number of Virtual Lanes to use (1-8)");

static uint eager_buffer_size;
module_param(eager_buffer_size, uint, S_IRUGO);
MODULE_PARM_DESC(eager_buffer_size, "Size of the eager buffers, default max MTU`");

uint rcv_intr_timeout;
module_param(rcv_intr_timeout, uint, S_IRUGO);
MODULE_PARM_DESC(rcv_intr_timeout, "Receive interrupt mitigation timeout");

uint rcv_intr_count = 1;
module_param(rcv_intr_count, uint, S_IRUGO);
MODULE_PARM_DESC(rcv_intr_count, "Receive interrupt mitigation count");

ushort link_crc_mask = WFR_SUPPORTED_CRCS;
module_param(link_crc_mask, ushort, S_IRUGO);
MODULE_PARM_DESC(link_crc_mask, "CRCs to use on the link");

/* TODO: temporary */
static uint use_flr;
module_param_named(use_flr, use_flr, uint, S_IRUGO);
MODULE_PARM_DESC(use_flr, "Initialize the SPC with FLR");

/* TODO: temporary until the STL fabric manager is available to do this */
uint set_link_credits = 1;
module_param_named(set_link_credits, set_link_credits, uint, S_IRUGO);
MODULE_PARM_DESC(set_link_credits, "Set per-VL link credits so traffic will flow");

uint loopback;
module_param_named(loopback, loopback, uint, S_IRUGO);
MODULE_PARM_DESC(loopback, "Put into LCB loopback mode");

/* TODO: temporary; skip BCC steps */
uint disable_bcc = 0;
module_param_named(disable_bcc, disable_bcc, uint, S_IRUGO);
MODULE_PARM_DESC(disable_bcc, "Disable BCC steps in normal LinkUp");

uint nodma_rtail;
module_param(nodma_rtail, uint, S_IRUGO);
MODULE_PARM_DESC(use_flr, "0 for no DMA of hdr tail, 1 to DMA the hdr tail");

/* TODO: temporary */
static uint print_unimplemented = 1;
module_param_named(print_unimplemented, print_unimplemented, uint, S_IRUGO);
MODULE_PARM_DESC(print_unimplemented, "Have unimplemented functions print when called");

/* TODO: temporary */
#define EASY_LINKUP_UNSET 100
static uint sim_easy_linkup = EASY_LINKUP_UNSET;

static uint extended_psn = 0;
module_param(extended_psn, uint, S_IRUGO);
MODULE_PARM_DESC(extended_psn, "Use 24 or 31 bit PSN");

static unsigned sdma_idle_cnt = 64;
module_param_named(sdma_idle_cnt, sdma_idle_cnt, uint, S_IRUGO);
MODULE_PARM_DESC(sdma_idle_cnt, "sdma interrupt idle delay (default 64)");

static uint use_sdma = 1;
module_param(use_sdma, uint, S_IRUGO);
MODULE_PARM_DESC(use_sdma, "enable sdma traffic");

static uint enable_pkeys = 1;
module_param(enable_pkeys, uint, S_IRUGO);
MODULE_PARM_DESC(enable_pkeys, "Enable PKey checking on receive");

struct flag_table {
	u64 flag;	/* the flag */
	char *str;	/* description string */
	u16 extra;	/* extra information */
	u16 unused0;
	u32 unused1;
};

/* str must be a string constant */
#define FLAG_ENTRY(str, extra, flag) {flag, str, extra}

/* Send Error Consequences */
#define SEC_WRITE_DROPPED	0x1
#define SEC_PACKET_DROPPED	0x2
#define SEC_SC_HALTED		0x4	/* per-context only */
#define SEC_SPC_FREEZE		0x8	/* per-HFI only */

/* defines to build power on SC2VL table */
#define SC2VL_VAL( \
	num, \
	sc0, sc0val, \
	sc1, sc1val, \
	sc2, sc2val, \
	sc3, sc3val, \
	sc4, sc4val, \
	sc5, sc5val, \
	sc6, sc6val, \
	sc7, sc7val) \
( \
	((u64)(sc0val) << WFR_SEND_SC2VLT##num##_SC##sc0##_SHIFT) | \
	((u64)(sc1val) << WFR_SEND_SC2VLT##num##_SC##sc1##_SHIFT) | \
	((u64)(sc2val) << WFR_SEND_SC2VLT##num##_SC##sc2##_SHIFT) | \
	((u64)(sc3val) << WFR_SEND_SC2VLT##num##_SC##sc3##_SHIFT) | \
	((u64)(sc4val) << WFR_SEND_SC2VLT##num##_SC##sc4##_SHIFT) | \
	((u64)(sc5val) << WFR_SEND_SC2VLT##num##_SC##sc5##_SHIFT) | \
	((u64)(sc6val) << WFR_SEND_SC2VLT##num##_SC##sc6##_SHIFT) | \
	((u64)(sc7val) << WFR_SEND_SC2VLT##num##_SC##sc7##_SHIFT)   \
)

#define DC_SC_VL_VAL( \
	range, \
	e0, e0val, \
	e1, e1val, \
	e2, e2val, \
	e3, e3val, \
	e4, e4val, \
	e5, e5val, \
	e6, e6val, \
	e7, e7val, \
	e8, e8val, \
	e9, e9val, \
	e10, e10val, \
	e11, e11val, \
	e12, e12val, \
	e13, e13val, \
	e14, e14val, \
	e15, e15val) \
( \
	((u64)(e0val) << DCC_CFG_SC_VL_TABLE_##range##_ENTRY##e0##_SHIFT) | \
	((u64)(e1val) << DCC_CFG_SC_VL_TABLE_##range##_ENTRY##e1##_SHIFT) | \
	((u64)(e2val) << DCC_CFG_SC_VL_TABLE_##range##_ENTRY##e2##_SHIFT) | \
	((u64)(e3val) << DCC_CFG_SC_VL_TABLE_##range##_ENTRY##e3##_SHIFT) | \
	((u64)(e4val) << DCC_CFG_SC_VL_TABLE_##range##_ENTRY##e4##_SHIFT) | \
	((u64)(e5val) << DCC_CFG_SC_VL_TABLE_##range##_ENTRY##e5##_SHIFT) | \
	((u64)(e6val) << DCC_CFG_SC_VL_TABLE_##range##_ENTRY##e6##_SHIFT) | \
	((u64)(e7val) << DCC_CFG_SC_VL_TABLE_##range##_ENTRY##e7##_SHIFT) | \
	((u64)(e8val) << DCC_CFG_SC_VL_TABLE_##range##_ENTRY##e8##_SHIFT) | \
	((u64)(e9val) << DCC_CFG_SC_VL_TABLE_##range##_ENTRY##e9##_SHIFT) | \
	((u64)(e10val) << DCC_CFG_SC_VL_TABLE_##range##_ENTRY##e10##_SHIFT) | \
	((u64)(e11val) << DCC_CFG_SC_VL_TABLE_##range##_ENTRY##e11##_SHIFT) | \
	((u64)(e12val) << DCC_CFG_SC_VL_TABLE_##range##_ENTRY##e12##_SHIFT) | \
	((u64)(e13val) << DCC_CFG_SC_VL_TABLE_##range##_ENTRY##e13##_SHIFT) | \
	((u64)(e14val) << DCC_CFG_SC_VL_TABLE_##range##_ENTRY##e14##_SHIFT) | \
	((u64)(e15val) << DCC_CFG_SC_VL_TABLE_##range##_ENTRY##e15##_SHIFT) \
)

/*
 * TXE PIO Error flags and consequences
 */
static struct flag_table pio_err_status_flags[] = {
/* 0*/	FLAG_ENTRY("PioWriteBadCtxt",
		SEC_WRITE_DROPPED,
		WFR_SEND_PIO_ERR_STATUS_PIO_WRITE_BAD_CTXT_ERR_SMASK),
/* 1*/	FLAG_ENTRY("PioWriteAddrParity",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_WRITE_ADDR_PARITY_ERR_SMASK),
/* 2*/	FLAG_ENTRY("PioCsrParity",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_CSR_PARITY_ERR_SMASK),
/* 3*/	FLAG_ENTRY("PioSbMemFifo0",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_SB_MEM_FIFO0_ERR_SMASK),
/* 4*/	FLAG_ENTRY("PioSbMemFifo1",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_SB_MEM_FIFO1_ERR_SMASK),
/* 5*/	FLAG_ENTRY("PioPccFifoParity",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_PCC_FIFO_PARITY_ERR_SMASK),
/* 6*/	FLAG_ENTRY("PioPecFifoParity",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_PEC_FIFO_PARITY_ERR_SMASK),
/* 7*/	FLAG_ENTRY("PioSbrdctlCrrelParity",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_SBRDCTL_CRREL_PARITY_ERR_SMASK),
/* 8*/	FLAG_ENTRY("PioSbrdctrlCrrelFifoParity",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_SBRDCTRL_CRREL_FIFO_PARITY_ERR_SMASK),
/* 9*/	FLAG_ENTRY("PioPktEvictFifoParityErr",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_PKT_EVICT_FIFO_PARITY_ERR_SMASK),
/*10*/	FLAG_ENTRY("PioSmPktResetParity",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_SM_PKT_RESET_PARITY_ERR_SMASK),
/*11*/	FLAG_ENTRY("PioVlLenMemBank0Unc",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_VL_LEN_MEM_BANK0_UNC_ERR_SMASK),
/*12*/	FLAG_ENTRY("PioVlLenMemBank1Unc",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_VL_LEN_MEM_BANK1_UNC_ERR_SMASK),
/*13*/	FLAG_ENTRY("PioVlLenMemBank0Cor",
		0,
		WFR_SEND_PIO_ERR_STATUS_PIO_VL_LEN_MEM_BANK0_COR_ERR_SMASK),
/*14*/	FLAG_ENTRY("PioVlLenMemBank1Cor",
		0,
		WFR_SEND_PIO_ERR_STATUS_PIO_VL_LEN_MEM_BANK1_COR_ERR_SMASK),
/*15*/	FLAG_ENTRY("PioCreditRetFifoParity",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_CREDIT_RET_FIFO_PARITY_ERR_SMASK),
/*16*/	FLAG_ENTRY("PioPpmcPblFifo",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_PPMC_PBL_FIFO_ERR_SMASK),
/*17*/	FLAG_ENTRY("PioInitSmIn",
		0,
		WFR_SEND_PIO_ERR_STATUS_PIO_INIT_SM_IN_ERR_SMASK),
/*18*/	FLAG_ENTRY("PioPktEvictSmOrArbSm",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_PKT_EVICT_SM_OR_ARB_SM_ERR_SMASK),
/*19*/	FLAG_ENTRY("PioHostAddrMemUnc",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_HOST_ADDR_MEM_UNC_ERR_SMASK),
/*20*/	FLAG_ENTRY("PioHostAddrMemCor",
		0,
		WFR_SEND_PIO_ERR_STATUS_PIO_HOST_ADDR_MEM_COR_ERR_SMASK),
/*21*/	FLAG_ENTRY("PioWriteDataParity",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_WRITE_DATA_PARITY_ERR_SMASK),
/*22*/	FLAG_ENTRY("PioStateMachine",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_STATE_MACHINE_ERR_SMASK),
/*23*/	FLAG_ENTRY("PioWriteQwValidParity",
		SEC_WRITE_DROPPED|SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_WRITE_QW_VALID_PARITY_ERR_SMASK),
/*24*/	FLAG_ENTRY("PioBlockQwCountParity",
		SEC_WRITE_DROPPED|SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_BLOCK_QW_COUNT_PARITY_ERR_SMASK),
/*25*/	FLAG_ENTRY("PioVlfVlLenParity",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_VLF_VL_LEN_PARITY_ERR_SMASK),
/*26*/	FLAG_ENTRY("PioVlfSopParity",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_VLF_SOP_PARITY_ERR_SMASK),
/*27*/	FLAG_ENTRY("PioVlFifoParity",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_VL_FIFO_PARITY_ERR_SMASK),
/*28*/	FLAG_ENTRY("PioPpmcBqcMemParity",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_PPMC_BQC_MEM_PARITY_ERR_SMASK),
/*29*/	FLAG_ENTRY("PioPpmcSopLen",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_PPMC_SOP_LEN_ERR_SMASK),
/*30-31 reserved*/
/*32*/	FLAG_ENTRY("PioCurrentFreeCntParity",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_CURRENT_FREE_CNT_PARITY_ERR_SMASK),
/*33*/	FLAG_ENTRY("PioLastReturnedCntParity",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_LAST_RETURNED_CNT_PARITY_ERR_SMASK),
/*34*/	FLAG_ENTRY("PioPccSopHeadParity",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_PCC_SOP_HEAD_PARITY_ERR_SMASK),
/*35*/	FLAG_ENTRY("PioPecSopHeadParityErr",
		SEC_SPC_FREEZE,
		WFR_SEND_PIO_ERR_STATUS_PIO_PEC_SOP_HEAD_PARITY_ERR_SMASK),
/*36-63 reserved*/
};

/*
 * TXE Egress Error flags and consequences
 * TODO: Add consequences
 */
static struct flag_table egress_err_status_flags[] = {
/* 0*/	FLAG_ENTRY("TxPktIntegrityMemCorErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_PKT_INTEGRITY_MEM_COR_ERR_SMASK),
/* 1*/	FLAG_ENTRY("TxPktIntegrityMemUncErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_PKT_INTEGRITY_MEM_UNC_ERR_SMASK),
/* 2 reserved */
/* 3*/	FLAG_ENTRY("TxEgressFifoUnderrunOrParityErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_EGRESS_FIFO_UNDERRUN_OR_PARITY_ERR_SMASK),
/* 4*/	FLAG_ENTRY("TxLinkdownErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LINKDOWN_ERR_SMASK),
/* 5*/	FLAG_ENTRY("TxIncorrectLinkStateErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_INCORRECT_LINK_STATE_ERR_SMASK),
/* 6 reserved */
/* 7*/	FLAG_ENTRY("TxLaunchIntfParityErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_PIO_LAUNCH_INTF_PARITY_ERR_SMASK),
/* 8*/	FLAG_ENTRY("TxSdmaLaunchIntfParityErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SDMA_LAUNCH_INTF_PARITY_ERR_SMASK),
/* 9-10 reserved */
/*11*/	FLAG_ENTRY("TxSbrdCtlStateMachineParityErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SBRD_CTL_STATE_MACHINE_PARITY_ERR_SMASK),
#ifdef WFR_SEND_EGRESS_ERR_STATUS_TX_DC_PARITY_ERR_SMASK
/* TODO: sim v33 */
/*12*/	FLAG_ENTRY("TxDcParityErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_DC_PARITY_ERR_SMASK),
#else
/* TODO: sim v34 and higher */
/*12*/	FLAG_ENTRY("TxIllegalVLErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_ILLEGAL_VL_ERR_SMASK),
#endif
/*13*/	FLAG_ENTRY("TxLaunchCsrParityErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_CSR_PARITY_ERR_SMASK),
/*14*/	FLAG_ENTRY("TxSbrdCtlCsrParityErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SBRD_CTL_CSR_PARITY_ERR_SMASK),
/*15*/	FLAG_ENTRY("TxConfigParityErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_CONFIG_PARITY_ERR_SMASK),
/*16*/	FLAG_ENTRY("TxSdma0DisallowedPacketErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SDMA0_DISALLOWED_PACKET_ERR_SMASK),
/*17*/	FLAG_ENTRY("TxSdma1DisallowedPacketErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SDMA1_DISALLOWED_PACKET_ERR_SMASK),
/*18*/	FLAG_ENTRY("TxSdma2DisallowedPacketErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SDMA2_DISALLOWED_PACKET_ERR_SMASK),
/*19*/	FLAG_ENTRY("TxSdma3DisallowedPacketErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SDMA3_DISALLOWED_PACKET_ERR_SMASK),
/*20*/	FLAG_ENTRY("TxSdma4DisallowedPacketErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SDMA4_DISALLOWED_PACKET_ERR_SMASK),
/*21*/	FLAG_ENTRY("TxSdma5DisallowedPacketErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SDMA5_DISALLOWED_PACKET_ERR_SMASK),
/*22*/	FLAG_ENTRY("TxSdma6DisallowedPacketErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SDMA6_DISALLOWED_PACKET_ERR_SMASK),
/*23*/	FLAG_ENTRY("TxSdma7DisallowedPacketErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SDMA7_DISALLOWED_PACKET_ERR_SMASK),
/*24*/	FLAG_ENTRY("TxSdma8DisallowedPacketErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SDMA8_DISALLOWED_PACKET_ERR_SMASK),
/*25*/	FLAG_ENTRY("TxSdma9DisallowedPacketErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SDMA9_DISALLOWED_PACKET_ERR_SMASK),
/*26*/	FLAG_ENTRY("TxSdma10DisallowedPacketErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SDMA10_DISALLOWED_PACKET_ERR_SMASK),
/*27*/	FLAG_ENTRY("TxSdma11DisallowedPacketErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SDMA11_DISALLOWED_PACKET_ERR_SMASK),
/*28*/	FLAG_ENTRY("TxSdma12DisallowedPacketErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SDMA12_DISALLOWED_PACKET_ERR_SMASK),
/*29*/	FLAG_ENTRY("TxSdma13DisallowedPacketErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SDMA13_DISALLOWED_PACKET_ERR_SMASK),
/*30*/	FLAG_ENTRY("TxSdma14DisallowedPacketErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SDMA14_DISALLOWED_PACKET_ERR_SMASK),
/*31*/	FLAG_ENTRY("TxSdma15DisallowedPacketErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SDMA15_DISALLOWED_PACKET_ERR_SMASK),
/*32*/	FLAG_ENTRY("TxLaunchFifo0UncOrParityErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_FIFO0_UNC_OR_PARITY_ERR_SMASK),
/*33*/	FLAG_ENTRY("TxLaunchFifo1UncOrParityErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_FIFO1_UNC_OR_PARITY_ERR_SMASK),
/*34*/	FLAG_ENTRY("TxLaunchFifo2UncOrParityErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_FIFO2_UNC_OR_PARITY_ERR_SMASK),
/*35*/	FLAG_ENTRY("TxLaunchFifo3UncOrParityErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_FIFO3_UNC_OR_PARITY_ERR_SMASK),
/*36*/	FLAG_ENTRY("TxLaunchFifo4UncOrParityErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_FIFO4_UNC_OR_PARITY_ERR_SMASK),
/*37*/	FLAG_ENTRY("TxLaunchFifo5UncOrParityErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_FIFO5_UNC_OR_PARITY_ERR_SMASK),
/*38*/	FLAG_ENTRY("TxLaunchFifo6UncOrParityErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_FIFO6_UNC_OR_PARITY_ERR_SMASK),
/*39*/	FLAG_ENTRY("TxLaunchFifo7UncOrParityErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_FIFO7_UNC_OR_PARITY_ERR_SMASK),
/*40*/	FLAG_ENTRY("TxLaunchFifo8UncOrParityErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_FIFO8_UNC_OR_PARITY_ERR_SMASK),
/*41*/	FLAG_ENTRY("TxCreditReturnParityErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_CREDIT_RETURN_PARITY_ERR_SMASK),
/*42*/	FLAG_ENTRY("TxSbHdrUncErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SB_HDR_UNC_ERR_SMASK),
/*43*/	FLAG_ENTRY("TxReadSdmaMemoryUncErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_READ_SDMA_MEMORY_UNC_ERR_SMASK),
/*44*/	FLAG_ENTRY("TxReadPioMemoryUncErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_READ_PIO_MEMORY_UNC_ERR_SMASK),
/*45*/	FLAG_ENTRY("TxEgressFifoUncErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_EGRESS_FIFO_UNC_ERR_SMASK),
/*46*/	FLAG_ENTRY("TxHcrcInsertionErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_HCRC_INSERTION_ERR_SMASK),
/*47*/	FLAG_ENTRY("TxCreditReturnVLErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_CREDIT_RETURN_VL_ERR_SMASK),
/*48*/	FLAG_ENTRY("TxLaunchFifo0CorErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_FIFO0_COR_ERR_SMASK),
/*49*/	FLAG_ENTRY("TxLaunchFifo1CorErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_FIFO1_COR_ERR_SMASK),
/*50*/	FLAG_ENTRY("TxLaunchFifo2CorErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_FIFO2_COR_ERR_SMASK),
/*51*/	FLAG_ENTRY("TxLaunchFifo3CorErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_FIFO3_COR_ERR_SMASK),
/*52*/	FLAG_ENTRY("TxLaunchFifo4CorErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_FIFO4_COR_ERR_SMASK),
/*53*/	FLAG_ENTRY("TxLaunchFifo5CorErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_FIFO5_COR_ERR_SMASK),
/*54*/	FLAG_ENTRY("TxLaunchFifo6CorErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_FIFO6_COR_ERR_SMASK),
/*55*/	FLAG_ENTRY("TxLaunchFifo7CorErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_FIFO7_COR_ERR_SMASK),
/*56*/	FLAG_ENTRY("TxLaunchFifo8CorErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_LAUNCH_FIFO8_COR_ERR_SMASK),
/*57*/	FLAG_ENTRY("TxCreditOverrunErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_CREDIT_OVERRUN_ERR_SMASK),
/*58*/	FLAG_ENTRY("TxSbHdrCorErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_SB_HDR_COR_ERR_SMASK),
/*59*/	FLAG_ENTRY("TxReadSdmaMemoryCorErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_READ_SDMA_MEMORY_COR_ERR_SMASK),
/*60*/	FLAG_ENTRY("TxReadPioMemoryCorErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_READ_PIO_MEMORY_COR_ERR_SMASK),
/*61*/	FLAG_ENTRY("TxEgressFifoCorErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_EGRESS_FIFO_COR_ERR_SMASK),
/*62*/	FLAG_ENTRY("TxReadSdmaMemoryCsrUncErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_READ_SDMA_MEMORY_CSR_UNC_ERR_SMASK),
/*63*/	FLAG_ENTRY("TxReadPioMemoryCsrUncErr",
		0,
		WFR_SEND_EGRESS_ERR_STATUS_TX_READ_PIO_MEMORY_CSR_UNC_ERR_SMASK),
};

/*
 * TXE Send Context Error flags and consequences
 */
static struct flag_table sc_err_status_flags[] = {
/* 0*/	FLAG_ENTRY("InconsistentSop",
		SEC_PACKET_DROPPED | SEC_SC_HALTED,
		WFR_SEND_CTXT_ERR_STATUS_PIO_INCONSISTENT_SOP_ERR_SMASK),
/* 1*/	FLAG_ENTRY("DisallowedPacket",
		SEC_PACKET_DROPPED | SEC_SC_HALTED,
		WFR_SEND_CTXT_ERR_STATUS_PIO_DISALLOWED_PACKET_ERR_SMASK),
/* 2*/	FLAG_ENTRY("WriteCrossesBoundary",
		SEC_WRITE_DROPPED | SEC_SC_HALTED,
		WFR_SEND_CTXT_ERR_STATUS_PIO_WRITE_CROSSES_BOUNDARY_ERR_SMASK),
/* 3*/	FLAG_ENTRY("WriteOverflow",
		SEC_WRITE_DROPPED | SEC_SC_HALTED,
		WFR_SEND_CTXT_ERR_STATUS_PIO_WRITE_OVERFLOW_ERR_SMASK),
/* 4*/	FLAG_ENTRY("WriteOutOfBounds",
		SEC_WRITE_DROPPED | SEC_SC_HALTED,
		WFR_SEND_CTXT_ERR_STATUS_PIO_WRITE_OUT_OF_BOUNDS_ERR_SMASK),
/* 5-63 reserved*/
};

/*
 * Credit Return flags
 */
static struct flag_table credit_return_flags[] = {
	FLAG_ENTRY("Status", 0, WFR_CR_STATUS_SMASK),
	FLAG_ENTRY("CreditReturnDueToPbc", 0,
		WFR_CR_CREDIT_RETURN_DUE_TO_PBC_SMASK),
	FLAG_ENTRY("CreditReturnDueToThreshold", 0,
		WFR_CR_CREDIT_RETURN_DUE_TO_THRESHOLD_SMASK),
	FLAG_ENTRY("CreditReturnDueToErr", 0,
		WFR_CR_CREDIT_RETURN_DUE_TO_ERR_SMASK),
	FLAG_ENTRY("CreditReturnDueToForce", 0,
		WFR_CR_CREDIT_RETURN_DUE_TO_FORCE_SMASK)
};

/*
 * DCC Error Flags
 */
static struct flag_table dcc_err_flags[] = {
	FLAG_ENTRY("bad_l2_err", 0,
		DCC_ERR_FLG_BAD_L2_ERR_SMASK),
	FLAG_ENTRY("bad_sc_err", 0,
		DCC_ERR_FLG_BAD_SC_ERR_SMASK),
	FLAG_ENTRY("bad_mid_tail_err", 0,
		DCC_ERR_FLG_BAD_MID_TAIL_ERR_SMASK),
	FLAG_ENTRY("bad_preemption_err", 0,
		DCC_ERR_FLG_BAD_PREEMPTION_ERR_SMASK),
	FLAG_ENTRY("preemption_err", 0,
		DCC_ERR_FLG_PREEMPTION_ERR_SMASK),
	FLAG_ENTRY("preemptionvl15_err", 0,
		DCC_ERR_FLG_PREEMPTIONVL15_ERR_SMASK),
	FLAG_ENTRY("bad_vl_marker_err", 0,
		DCC_ERR_FLG_BAD_VL_MARKER_ERR_SMASK),
	FLAG_ENTRY("bad_dlid_target_err", 0,
		DCC_ERR_FLG_BAD_DLID_TARGET_ERR_SMASK),
	FLAG_ENTRY("bad_lver_err", 0,
		DCC_ERR_FLG_BAD_LVER_ERR_SMASK),
	FLAG_ENTRY("uncorrectable_err", 0,
		DCC_ERR_FLG_UNCORRECTABLE_ERR_SMASK),
	FLAG_ENTRY("bad_crdt_ack_err", 0,
		DCC_ERR_FLG_BAD_CRDT_ACK_ERR_SMASK),
	FLAG_ENTRY("unsup_pkt_type", 0,
		DCC_ERR_FLG_UNSUP_PKT_TYPE_SMASK),
	FLAG_ENTRY("bad_ctrl_flit_err", 0,
		DCC_ERR_FLG_BAD_CTRL_FLIT_ERR_SMASK),
	FLAG_ENTRY("event_cntr_parity_err", 0,
		DCC_ERR_FLG_EVENT_CNTR_PARITY_ERR_SMASK),
	FLAG_ENTRY("event_cntr_rollover_err", 0,
		DCC_ERR_FLG_EVENT_CNTR_ROLLOVER_ERR_SMASK),
	FLAG_ENTRY("link_err", 0,
		DCC_ERR_FLG_LINK_ERR_SMASK),
	FLAG_ENTRY("misc_cntr_rollover_err", 0,
		DCC_ERR_FLG_MISC_CNTR_ROLLOVER_ERR_SMASK),
	FLAG_ENTRY("bad_ctrl_dist_err", 0,
		DCC_ERR_FLG_BAD_CTRL_DIST_ERR_SMASK),
	FLAG_ENTRY("bad_tail_dist_err", 0,
		DCC_ERR_FLG_BAD_TAIL_DIST_ERR_SMASK),
	FLAG_ENTRY("bad_head_dist_err", 0,
		DCC_ERR_FLG_BAD_HEAD_DIST_ERR_SMASK),
	FLAG_ENTRY("nonvl15_state_err", 0,
		DCC_ERR_FLG_NONVL15_STATE_ERR_SMASK),
	FLAG_ENTRY("vl15_multi_err", 0,
		DCC_ERR_FLG_VL15_MULTI_ERR_SMASK),
	FLAG_ENTRY("bad_pkt_length_err", 0,
		DCC_ERR_FLG_BAD_PKT_LENGTH_ERR_SMASK),
	FLAG_ENTRY("unsup_vl_err", 0,
		DCC_ERR_FLG_UNSUP_VL_ERR_SMASK),
	FLAG_ENTRY("perm_nvl15_err", 0,
		DCC_ERR_FLG_PERM_NVL15_ERR_SMASK),
	FLAG_ENTRY("slid_zero_err", 0,
		DCC_ERR_FLG_SLID_ZERO_ERR_SMASK),
	FLAG_ENTRY("dlid_zero_err", 0,
		DCC_ERR_FLG_DLID_ZERO_ERR_SMASK),
	FLAG_ENTRY("length_mtu_err", 0,
		DCC_ERR_FLG_LENGTH_MTU_ERR_SMASK),
	FLAG_ENTRY("rx_early_drop_err", 0,
		DCC_ERR_FLG_RX_EARLY_DROP_ERR_SMASK),
	FLAG_ENTRY("late_short_err", 0,
		DCC_ERR_FLG_LATE_SHORT_ERR_SMASK),
	FLAG_ENTRY("late_long_err", 0,
		DCC_ERR_FLG_LATE_LONG_ERR_SMASK),
	FLAG_ENTRY("late_ebp_err", 0,
		DCC_ERR_FLG_LATE_EBP_ERR_SMASK),
	FLAG_ENTRY("fpe_tx_fifo_ovflw_err", 0,
		DCC_ERR_FLG_FPE_TX_FIFO_OVFLW_ERR_SMASK),
	FLAG_ENTRY("fpe_tx_fifo_unflw_err", 0,
		DCC_ERR_FLG_FPE_TX_FIFO_UNFLW_ERR_SMASK),
	FLAG_ENTRY("csr_access_blocked_host", 0,
		DCC_ERR_FLG_CSR_ACCESS_BLOCKED_HOST_SMASK),
	FLAG_ENTRY("csr_access_blocked_uc", 0,
		DCC_ERR_FLG_CSR_ACCESS_BLOCKED_UC_SMASK),
	FLAG_ENTRY("tx_ctrl_parity_err", 0,
		DCC_ERR_FLG_TX_CTRL_PARITY_ERR_SMASK),
	FLAG_ENTRY("tx_ctrl_parity_mbe_err", 0,
		DCC_ERR_FLG_TX_CTRL_PARITY_MBE_ERR_SMASK),
	FLAG_ENTRY("tx_sc_parity_err", 0,
		DCC_ERR_FLG_TX_SC_PARITY_ERR_SMASK),
	FLAG_ENTRY("rx_ctrl_parity_mbe_err", 0,
		DCC_ERR_FLG_RX_CTRL_PARITY_MBE_ERR_SMASK),
	FLAG_ENTRY("csr_parity_err", 0,
		DCC_ERR_FLG_CSR_PARITY_ERR_SMASK),
	FLAG_ENTRY("csr_inval_addr", 0,
		DCC_ERR_FLG_CSR_INVAL_ADDR_SMASK),
	FLAG_ENTRY("tx_byte_shft_parity_err", 0,
		DCC_ERR_FLG_TX_BYTE_SHFT_PARITY_ERR_SMASK),
	FLAG_ENTRY("rx_byte_shft_parity_err", 0,
		DCC_ERR_FLG_RX_BYTE_SHFT_PARITY_ERR_SMASK),
	FLAG_ENTRY("fmconfig_err", 0,
		DCC_ERR_FLG_FMCONFIG_ERR_SMASK),
	FLAG_ENTRY("rcvport_err", 0,
		DCC_ERR_FLG_RCVPORT_ERR_SMASK),
};

/*
 * DC8051 Error Flags
 */
static struct flag_table dc8051_err_flags[] = {
	FLAG_ENTRY("SET_BY_8051", 0,
		DC_DC8051_ERR_FLG_SET_BY_8051_SMASK),
	FLAG_ENTRY("LOST_8051_HEART_BEAT", 0,
		DC_DC8051_ERR_FLG_LOST_8051_HEART_BEAT_SMASK),
	FLAG_ENTRY("CRAM_MBE", 0,
		DC_DC8051_ERR_FLG_CRAM_MBE_SMASK),
	FLAG_ENTRY("CRAM_SBE", 0,
		DC_DC8051_ERR_FLG_CRAM_SBE_SMASK),
	FLAG_ENTRY("DRAM_MBE", 0,
		DC_DC8051_ERR_FLG_DRAM_MBE_SMASK),
	FLAG_ENTRY("DRAM_SBE", 0,
		DC_DC8051_ERR_FLG_DRAM_SBE_SMASK),
	FLAG_ENTRY("IRAM_MBE", 0,
		DC_DC8051_ERR_FLG_IRAM_MBE_SMASK),
	FLAG_ENTRY("IRAM_SBE", 0,
		DC_DC8051_ERR_FLG_IRAM_SBE_SMASK),
	FLAG_ENTRY("UNMATCHED_SECURE_MSG_ACROSS_BCC_LANES", 0,
		DC_DC8051_ERR_FLG_UNMATCHED_SECURE_MSG_ACROSS_BCC_LANES_SMASK),
	FLAG_ENTRY("INVALID_CSR_ADDR", 0,
		DC_DC8051_ERR_FLG_INVALID_CSR_ADDR_SMASK),
};

/*
 * DC8051 Information Error flags
 *
 * Flags in DC8051_DBG_ERR_INFO_SET_BY_8051.ERROR field.
 */
static struct flag_table dc8051_info_err_flags[] = {
	FLAG_ENTRY("Spico ROM check failed",		0, 0x0001),
	FLAG_ENTRY("UNKNOWN_FRAME type",		0, 0x0002),
	FLAG_ENTRY("Target BER not met",		0, 0x0004),
	FLAG_ENTRY("Serdes internal looopback failure", 0, 0x0008),
	FLAG_ENTRY("Failed SerDes Init",		0, 0x0010),
	FLAG_ENTRY("Failed LNI(Polling)",		0, 0x0020),
	FLAG_ENTRY("Failed LNI(Debounce)",		0, 0x0040),
	FLAG_ENTRY("Failed LNI(EstbComm)",		0, 0x0080),
	FLAG_ENTRY("Failed LNI(OptEq)",			0, 0x0100),
	FLAG_ENTRY("Failed LNI(VerifyCap_1)",		0, 0x0200),
	FLAG_ENTRY("Failed LNI(VerifyCap_2)",		0, 0x0400),
	FLAG_ENTRY("Failed LNI(ConfigLT)",		0, 0x0800)
};

/*
 * DC8051 Information Host Information flags
 *
 * Flags in DC8051_DBG_ERR_INFO_SET_BY_8051.HOST_MSG field.
 */
static struct flag_table dc8051_info_host_msg_flags[] = {
	FLAG_ENTRY("Host request done",			0, 0x0001),
	FLAG_ENTRY("BC SMA message",			0, 0x0002),
	FLAG_ENTRY("BC PWR_MGM message",		0, 0x0004),
	FLAG_ENTRY("BC Unknown message (BCC)",		0, 0x0008),
	FLAG_ENTRY("BC Unknown message (LCB)",		0, 0x0010),
	FLAG_ENTRY("External device config request",	0, 0x0020),
	FLAG_ENTRY("VerifyCap all frames received",	0, 0x0040),
	FLAG_ENTRY("LinkUp achieved",			0, 0x0080),
	FLAG_ENTRY("Link going down",			0, 0x0100),
};


static u32 encoded_size(u32 size);
static u32 chip_to_ib_lstate(struct hfi_devdata *dd, u32 chip_lstate);
static int set_physical_link_state(struct hfi_devdata *dd, u64 state);
static void read_vc_remote_phy(struct hfi_devdata *dd, u8 *power_management,
				u8 *continous);
static void read_vc_remote_fabric(struct hfi_devdata *dd, u8 *vau, u8 *vcu,
				u16 *vl15buf, u8 *crc_sizes);
static void read_vc_remote_link_width(struct hfi_devdata *dd, u16 *flag_bits,
				u16 *link_widths);
static void handle_send_context_err(struct hfi_devdata *dd,
				unsigned int context, u64 err_status);
static void handle_sdma_eng_err(struct hfi_devdata *dd,
				unsigned int context, u64 err_status);
static void handle_dcc_err(struct hfi_devdata *dd,
				unsigned int context, u64 err_status);
static void handle_lcb_err(struct hfi_devdata *dd,
				unsigned int context, u64 err_status);
static void handle_8051_interrupt(struct hfi_devdata *dd, u32 unused, u64 reg);
static void handle_cce_err(struct hfi_devdata *dd, u32 unused, u64 reg);
static void handle_rxe_err(struct hfi_devdata *dd, u32 unused, u64 reg);
static void handle_misc_err(struct hfi_devdata *dd, u32 unused, u64 reg);
static void handle_pio_err(struct hfi_devdata *dd, u32 unused, u64 reg);
static void handle_sdma_err(struct hfi_devdata *dd, u32 unused, u64 reg);
static void handle_egress_err(struct hfi_devdata *dd, u32 unused, u64 reg);
static void handle_txe_err(struct hfi_devdata *dd, u32 unused, u64 reg);
static void set_partition_keys(struct qib_pportdata *);

/*
 * Error interrupt table entry.  This is used as input to the interrupt
 * "clear down" routine used for all second tier error interrupt register.
 * Second tier interrupt registers have a single bit representing them
 * in the top-level CceIntStatus.
 */
struct err_reg_info {
	u32 status;		/* status CSR offset */
	u32 clear;		/* clear CSR offset */
	u32 mask;		/* mask CSR offset */
	void (*handler)(struct hfi_devdata *dd, u32 source, u64 reg);
	const char *desc;
};

#define NUM_MISC_ERRS (WFR_IS_GENERAL_ERR_END - WFR_IS_GENERAL_ERR_START)
#define NUM_DC_ERRS (WFR_IS_DC_END - WFR_IS_DC_START)

/*
 * Helpers for building WFR and DC error interrupt table entries.  Different
 * helpers are needed because of inconsistent register names.
 */
#define WFR_EE(reg, handler, desc) \
	{ WFR_##reg##_STATUS, WFR_##reg##_CLEAR, WFR_##reg##_MASK, \
		handler, desc }
#define DC_EE1(reg, handler, desc) \
	{ reg##_FLG, reg##_FLG_CLR, reg##_FLG_EN, handler, desc }
#define DC_EE2(reg, handler, desc) \
	{ reg##_FLG, reg##_CLR, reg##_EN, handler, desc }

/*
 * Table of the "misc" grouping of error interrupts.  Each entry refers to
 * another register containing more information.
 */
static const struct err_reg_info misc_errs[NUM_MISC_ERRS] = {
/* 0*/	WFR_EE(CCE_ERR,		handle_cce_err,    "CceErr"),
/* 1*/	WFR_EE(RCV_ERR,		handle_rxe_err,    "RxeErr"),
/* 2*/	WFR_EE(MISC_ERR,	handle_misc_err,   "MiscErr"),
/* 3*/	{ 0, 0, 0, 0 }, /* reserved */
/* 4*/	WFR_EE(SEND_PIO_ERR,    handle_pio_err,    "PioErr"),
/* 5*/	WFR_EE(SEND_DMA_ERR,    handle_sdma_err,   "SDmaErr"),
/* 6*/	WFR_EE(SEND_EGRESS_ERR, handle_egress_err, "EgressErr"),
/* 7*/	WFR_EE(SEND_ERR,	handle_txe_err,    "TxeErr")
	/* the rest are reserved */
};

/*
 * Send context error interupt entry - refers to another register
 * containing more information.
 */
static const struct err_reg_info sendctxt_err =
	WFR_EE(SEND_CTXT_ERR,	handle_send_context_err, "SendCtxtErr");

/*
 * SDMA error interupt entry - refers to another register containing more
 * information.
 */
static const struct err_reg_info sdma_eng_err =
	WFR_EE(SEND_DMA_ENG_ERR, handle_sdma_eng_err, "SDmaEngErr");

/*
 * The DC encoding of mtu_cap for 10K MTU in the DCC_CFG_PORT_CONFIG
 * register can not be derived from the MTU value because 10K is not
 * a power of 2. Therefore, we need a constant. Everything else can
 * be calculated.
 */
#define DCC_CFG_PORT_MTU_CAP_10240 7

/*
 * Table of the DC grouping of error interupts.  Each entry refers to
 * another register containing more information.
 */
static const struct err_reg_info dc_errs[NUM_DC_ERRS] = {
/* 0*/	DC_EE1(DCC_ERR,		handle_dcc_err,	       "DCC Err"),
/* 1*/	DC_EE2(DC_LCB_ERR,	handle_lcb_err,	       "LCB Err"),
/* 2*/	DC_EE2(DC_DC8051_ERR,	handle_8051_interrupt, "DC8051 Interrupt"),
/* 3*/	/* dc_lbm_int - special, see is_dc_int() */
	/* the rest are reserved */
};

struct cntr_entry {
	/*
	 * stat name
	 */
	char *name;
	/*
	 * csr for name
	 */
	u64 csr;
	/*
	 * offset synthetic counter into dd or ppd
	 */
	size_t offset;
	/*
	 * reader for stat element, context either dd or ppd
	 */
	u64 (*read_cntr)(const struct cntr_entry *, void *context);
};

#define CNTR_ELEM(name, csr, offset, read) \
{ \
	name, \
	csr, \
	offset, \
	read \
}

#define RXE64_DEV_CNTR_ELEM(name, counter) \
CNTR_ELEM(#name , \
	  (counter * 8 + WFR_RCV_COUNTER_ARRAY64), \
	  0 , \
	  dev_read_u64_csr)

#define RXE64_PORT_CNTR_ELEM(name, counter) \
CNTR_ELEM(#name , \
	  (counter * 8 + WFR_RCV_COUNTER_ARRAY64), \
	  0 , \
	  port_read_u64_csr)

#define RXE32_DEV_CNTR_ELEM(name, counter) \
CNTR_ELEM(#name , \
	  (counter * 8 + WFR_RCV_COUNTER_ARRAY32), \
	  0 , \
	  dev_read_u32_csr)

#define CCE_PERF_DEV_CNTR_ELEM(name, counter) \
CNTR_ELEM(#name , \
	  (counter * 8 + WFR_CCE_COUNTER_ARRAY32), \
	  0 , \
	  dev_read_u32_csr)

#define CCE_INT_DEV_CNTR_ELEM(name, counter) \
CNTR_ELEM(#name , \
	  (counter * 8 + WFR_CCE_INT_COUNTER_ARRAY32), \
	  0 , \
	  dev_read_u32_csr)

#define RXE32_PORT_CNTR_ELEM(name, counter) \
CNTR_ELEM(#name , \
	  (counter * 8 + WFR_RCV_COUNTER_ARRAY32), \
	  0 , \
	  port_read_u32_csr)

#define TXE32_PORT_CNTR_ELEM(name, counter) \
CNTR_ELEM(#name , \
	  (counter * 8 + WFR_SEND_COUNTER_ARRAY32), \
	  0 , \
	  port_read_u32_csr)

#define TXE64_PORT_CNTR_ELEM(name, counter) \
CNTR_ELEM(#name , \
	  (counter * 8 + WFR_SEND_COUNTER_ARRAY64), \
	  0 , \
	  port_read_u64_csr)

#define OVERFLOW_ELEM(ctx) \
CNTR_ELEM("RcvHdrOvr" #ctx, \
	  (WFR_RCV_HDR_OVFL_CNT + ctx*0x100), \
	  0, port_read_u64_csr)



u64 read_csr(const struct hfi_devdata *dd, u32 offset)
{
	u64 val;

	if (dd->flags & QIB_PRESENT) {
		val = readq((void *)dd->kregbase + offset);
		return le64_to_cpu(val);
	}
	return -1;
}

void write_csr(const struct hfi_devdata *dd, u32 offset, u64 value)
{
	if (dd->flags & QIB_PRESENT)
		writeq(cpu_to_le64(value), (void *)dd->kregbase + offset);
}

static u64 dev_read_u32_csr(const struct cntr_entry *entry, void *context)
{
	struct hfi_devdata *dd = (struct hfi_devdata *)context;

	return read_csr(dd, entry->csr);
}

static u64 port_read_u64_csr(const struct cntr_entry *entry, void *context)
{
	struct qib_pportdata *ppd = (struct qib_pportdata *)context;

	return read_csr(ppd->dd, entry->csr);
}

static u64 port_read_u32_csr(const struct cntr_entry *entry, void *context)
{
	struct qib_pportdata *ppd = (struct qib_pportdata *)context;

	return read_csr(ppd->dd, entry->csr);
}

static const struct cntr_entry dev_cntrs[] = {
RXE32_DEV_CNTR_ELEM(RxTIDFullEr, RCV_TID_FULL_ERR_CNT),
RXE32_DEV_CNTR_ELEM(RxTIDInvalid, RCV_TID_VALID_ERR_CNT),
RXE32_DEV_CNTR_ELEM(RxTidFLGMs, RCV_TID_FLOW_GEN_MISMATCH_CNT),
RXE32_DEV_CNTR_ELEM(RxCtxRHQS, RCV_CONTEXT_RHQ_STALL),
RXE32_DEV_CNTR_ELEM(RxCtxEgrS, RCV_CONTEXT_EGR_STALL),
RXE32_DEV_CNTR_ELEM(RxTidFLSMs, RCV_TID_FLOW_SEQ_MISMATCH_CNT),
CCE_PERF_DEV_CNTR_ELEM(CcePciCrSt, CCE_PCIE_POSTED_CRDT_STALL_CNT),
CCE_PERF_DEV_CNTR_ELEM(CcePciTrSt, CCE_PCIE_TRGT_STALL_CNT),
CCE_PERF_DEV_CNTR_ELEM(CcePioWrSt, CCE_PIO_WR_STALL_CNT),
CCE_INT_DEV_CNTR_ELEM(CceErrInt, CCE_ERR_INT_CNT),
CCE_INT_DEV_CNTR_ELEM(CceSdmaInt, CCE_SDMA_INT_CNT),
CCE_INT_DEV_CNTR_ELEM(CceMiscInt, CCE_MISC_INT_CNT),
CCE_INT_DEV_CNTR_ELEM(CceRcvAvInt, CCE_RCV_AVAIL_INT_CNT),
CCE_INT_DEV_CNTR_ELEM(CceRcvUrgInt, CCE_RCV_URGENT_INT_CNT),
CCE_INT_DEV_CNTR_ELEM(CceSndCrInt, CCE_SEND_CREDIT_INT_CNT),
};

static const struct cntr_entry port_cntrs[] = {
TXE32_PORT_CNTR_ELEM(TxUnVLErr, SEND_UNSUP_VL_ERR_CNT),
TXE32_PORT_CNTR_ELEM(TxInvalLen, SEND_LEN_ERR_CNT),
TXE32_PORT_CNTR_ELEM(TxMMLenErr, SEND_MAX_MIN_LEN_ERR_CNT),
TXE32_PORT_CNTR_ELEM(TxUnderrun, SEND_UNDERRUN_CNT),
TXE32_PORT_CNTR_ELEM(TxFlowStall, SEND_FLOW_STALL_CNT),
TXE32_PORT_CNTR_ELEM(TxDropped, SEND_DROPPED_PKT_CNT),
TXE32_PORT_CNTR_ELEM(TxHdrErr, SEND_HEADERS_ERR_CNT),
TXE64_PORT_CNTR_ELEM(TxPkt, SEND_DATA_PKT_CNT),
TXE64_PORT_CNTR_ELEM(TxWords, SEND_DWORD_CNT),
TXE64_PORT_CNTR_ELEM(TxWait, SEND_WAIT_CNT),
TXE64_PORT_CNTR_ELEM(TxFlitVL0, SEND_DATA_VL0_CNT),
TXE64_PORT_CNTR_ELEM(TxFlitVL1, SEND_DATA_VL1_CNT),
TXE64_PORT_CNTR_ELEM(TxFlitVL2, SEND_DATA_VL2_CNT),
TXE64_PORT_CNTR_ELEM(TxFlitVL3, SEND_DATA_VL3_CNT),
TXE64_PORT_CNTR_ELEM(TxFlitVL4, SEND_DATA_VL4_CNT),
TXE64_PORT_CNTR_ELEM(TxFlitVL5, SEND_DATA_VL5_CNT),
TXE64_PORT_CNTR_ELEM(TxFlitVL6, SEND_DATA_VL6_CNT),
TXE64_PORT_CNTR_ELEM(TxFlitVL7, SEND_DATA_VL7_CNT),
TXE64_PORT_CNTR_ELEM(TxFlitVL15, SEND_DATA_VL15_CNT),
TXE64_PORT_CNTR_ELEM(TxPktVL0, SEND_DATA_PKT_VL0_CNT),
TXE64_PORT_CNTR_ELEM(TxPktVL1, SEND_DATA_PKT_VL1_CNT),
TXE64_PORT_CNTR_ELEM(TxPktVL2, SEND_DATA_PKT_VL2_CNT),
TXE64_PORT_CNTR_ELEM(TxPktVL3, SEND_DATA_PKT_VL3_CNT),
TXE64_PORT_CNTR_ELEM(TxPktVL4, SEND_DATA_PKT_VL4_CNT),
TXE64_PORT_CNTR_ELEM(TxPktVL5, SEND_DATA_PKT_VL5_CNT),
TXE64_PORT_CNTR_ELEM(TxPktVL6, SEND_DATA_PKT_VL6_CNT),
TXE64_PORT_CNTR_ELEM(TxPktVL7, SEND_DATA_PKT_VL7_CNT),
TXE64_PORT_CNTR_ELEM(TxPktVL15, SEND_DATA_PKT_VL15_CNT),
TXE64_PORT_CNTR_ELEM(TxWaitVL0, SEND_WAIT_VL0_CNT),
TXE64_PORT_CNTR_ELEM(TxWaitVL1, SEND_WAIT_VL1_CNT),
TXE64_PORT_CNTR_ELEM(TxWaitVL2, SEND_WAIT_VL2_CNT),
TXE64_PORT_CNTR_ELEM(TxWaitVL3, SEND_WAIT_VL3_CNT),
TXE64_PORT_CNTR_ELEM(TxWaitVL4, SEND_WAIT_VL4_CNT),
TXE64_PORT_CNTR_ELEM(TxWaitVL5, SEND_WAIT_VL5_CNT),
TXE64_PORT_CNTR_ELEM(TxWaitVL6, SEND_WAIT_VL6_CNT),
TXE64_PORT_CNTR_ELEM(TxWaitVL7, SEND_WAIT_VL7_CNT),
TXE64_PORT_CNTR_ELEM(TxWaitVL15, SEND_WAIT_VL15_CNT),
RXE64_PORT_CNTR_ELEM(RxPkt, RCV_DATA_PKT_CNT),
RXE64_PORT_CNTR_ELEM(RxWords, RCV_DWORD_CNT),
/* assumed to be last in array for sizing */
OVERFLOW_ELEM(0),   OVERFLOW_ELEM(1),   OVERFLOW_ELEM(2),   OVERFLOW_ELEM(3),
OVERFLOW_ELEM(4),   OVERFLOW_ELEM(5),   OVERFLOW_ELEM(6),   OVERFLOW_ELEM(7),
OVERFLOW_ELEM(8),   OVERFLOW_ELEM(9),   OVERFLOW_ELEM(10),  OVERFLOW_ELEM(11),
OVERFLOW_ELEM(12),  OVERFLOW_ELEM(13),  OVERFLOW_ELEM(14),  OVERFLOW_ELEM(15),
OVERFLOW_ELEM(16),  OVERFLOW_ELEM(17),  OVERFLOW_ELEM(18),  OVERFLOW_ELEM(19),
OVERFLOW_ELEM(20),  OVERFLOW_ELEM(21),  OVERFLOW_ELEM(22),  OVERFLOW_ELEM(23),
OVERFLOW_ELEM(24),  OVERFLOW_ELEM(25),  OVERFLOW_ELEM(26),  OVERFLOW_ELEM(27),
OVERFLOW_ELEM(28),  OVERFLOW_ELEM(29),  OVERFLOW_ELEM(30),  OVERFLOW_ELEM(31),
OVERFLOW_ELEM(32),  OVERFLOW_ELEM(33),  OVERFLOW_ELEM(34),  OVERFLOW_ELEM(35),
OVERFLOW_ELEM(36),  OVERFLOW_ELEM(37),  OVERFLOW_ELEM(38),  OVERFLOW_ELEM(39),
OVERFLOW_ELEM(40),  OVERFLOW_ELEM(41),  OVERFLOW_ELEM(42),  OVERFLOW_ELEM(43),
OVERFLOW_ELEM(44),  OVERFLOW_ELEM(45),  OVERFLOW_ELEM(46),  OVERFLOW_ELEM(47),
OVERFLOW_ELEM(48),  OVERFLOW_ELEM(49),  OVERFLOW_ELEM(50),  OVERFLOW_ELEM(51),
OVERFLOW_ELEM(52),  OVERFLOW_ELEM(53),  OVERFLOW_ELEM(54),  OVERFLOW_ELEM(55),
OVERFLOW_ELEM(56),  OVERFLOW_ELEM(57),  OVERFLOW_ELEM(58),  OVERFLOW_ELEM(59),
OVERFLOW_ELEM(60),  OVERFLOW_ELEM(61),  OVERFLOW_ELEM(62),  OVERFLOW_ELEM(63),
OVERFLOW_ELEM(64),  OVERFLOW_ELEM(65),  OVERFLOW_ELEM(66),  OVERFLOW_ELEM(67),
OVERFLOW_ELEM(68),  OVERFLOW_ELEM(69),  OVERFLOW_ELEM(70),  OVERFLOW_ELEM(71),
OVERFLOW_ELEM(72),  OVERFLOW_ELEM(73),  OVERFLOW_ELEM(74),  OVERFLOW_ELEM(75),
OVERFLOW_ELEM(76),  OVERFLOW_ELEM(77),  OVERFLOW_ELEM(78),  OVERFLOW_ELEM(79),
OVERFLOW_ELEM(80),  OVERFLOW_ELEM(81),  OVERFLOW_ELEM(82),  OVERFLOW_ELEM(83),
OVERFLOW_ELEM(84),  OVERFLOW_ELEM(85),  OVERFLOW_ELEM(86),  OVERFLOW_ELEM(87),
OVERFLOW_ELEM(88),  OVERFLOW_ELEM(89),  OVERFLOW_ELEM(90),  OVERFLOW_ELEM(91),
OVERFLOW_ELEM(92),  OVERFLOW_ELEM(93),  OVERFLOW_ELEM(94),  OVERFLOW_ELEM(95),
OVERFLOW_ELEM(96),  OVERFLOW_ELEM(97),  OVERFLOW_ELEM(98),  OVERFLOW_ELEM(99),
OVERFLOW_ELEM(100), OVERFLOW_ELEM(101), OVERFLOW_ELEM(102), OVERFLOW_ELEM(103),
OVERFLOW_ELEM(104), OVERFLOW_ELEM(105), OVERFLOW_ELEM(106), OVERFLOW_ELEM(107),
OVERFLOW_ELEM(108), OVERFLOW_ELEM(109), OVERFLOW_ELEM(110), OVERFLOW_ELEM(111),
OVERFLOW_ELEM(112), OVERFLOW_ELEM(113), OVERFLOW_ELEM(114), OVERFLOW_ELEM(115),
OVERFLOW_ELEM(116), OVERFLOW_ELEM(117), OVERFLOW_ELEM(118), OVERFLOW_ELEM(119),
OVERFLOW_ELEM(120), OVERFLOW_ELEM(121), OVERFLOW_ELEM(122), OVERFLOW_ELEM(123),
OVERFLOW_ELEM(124), OVERFLOW_ELEM(125), OVERFLOW_ELEM(126), OVERFLOW_ELEM(127),
OVERFLOW_ELEM(128), OVERFLOW_ELEM(129), OVERFLOW_ELEM(130), OVERFLOW_ELEM(131),
OVERFLOW_ELEM(132), OVERFLOW_ELEM(133), OVERFLOW_ELEM(134), OVERFLOW_ELEM(135),
OVERFLOW_ELEM(136), OVERFLOW_ELEM(137), OVERFLOW_ELEM(138), OVERFLOW_ELEM(139),
OVERFLOW_ELEM(140), OVERFLOW_ELEM(141), OVERFLOW_ELEM(142), OVERFLOW_ELEM(143),
OVERFLOW_ELEM(144), OVERFLOW_ELEM(145), OVERFLOW_ELEM(146), OVERFLOW_ELEM(147),
OVERFLOW_ELEM(148), OVERFLOW_ELEM(149), OVERFLOW_ELEM(150), OVERFLOW_ELEM(151),
OVERFLOW_ELEM(152), OVERFLOW_ELEM(153), OVERFLOW_ELEM(154), OVERFLOW_ELEM(155),
OVERFLOW_ELEM(156), OVERFLOW_ELEM(157), OVERFLOW_ELEM(158), OVERFLOW_ELEM(159),
};

/* ======================================================================== */

/*
 * Using the given flag table, print a comma separated string into
 * the buffer.  End in '*' if the buffer is too short.
 */
static char *flag_string(char *buf, int buf_len, u64 flags,
				struct flag_table *table, int table_size)
{
	const char *s;
	char *p = buf;
	int len = buf_len;
	int no_room = 0;
	int i;
	char c;

	/* make sure there is at least 2 so we can form "*" */
	if (len < 2)
		return "";

	len--;	/* leave room for a nul */
	for (i = 0; i < table_size; i++) {
		if (flags & table[i].flag) {
			/* add a comma, if not the first */
			if (p != buf) {
				if (len == 0) {
					no_room = 1;
					break;
				}
				*p++ = ',';
				len--;
			}
			/* copy the string */
			s = table[i].str;
			while ((c = *s++) != 0 && len > 0) {
				*p++ = c;
				len--;
			}
			if (c != 0) {
				no_room = 1;
				break;
			}
		}
	}
	/* add * if ran out of room */
	if (no_room) {
		/* may need to back up to add space for a '*' */
		if (len == 0)
			--p;
		*p++ = '*';
	}
	/* add final nul - space already allocated above */
	*p = 0;
	return buf;
}

/* first 8 CCE error interrupt source names */
static const char *cce_misc_names[] = {
	"CceErrInt",		/* 0 */
	"RxeErrInt",		/* 1 */
	"MiscErrInt",		/* 2 */
	"Reserved3",		/* 3 */
	"PioErrInt",		/* 4 */
	"SDmaErrInt",		/* 5 */
	"EgressErrInt",		/* 6 */
	"TxeErrInt"		/* 7 */
};

/*
 * Return the miscellaneous error interrupt name.
 */
static char *is_misc_err_name(char *buf, size_t bsize, unsigned int source)
{
	if (source < ARRAY_SIZE(cce_misc_names))
		strncpy(buf, cce_misc_names[source], bsize);
	else
		snprintf(buf, bsize, "Reserved%u", source + WFR_IS_GENERAL_ERR_START);

	return buf;
}

/*
 * Return the SDMA engine error interrupt name.
 */
static char *is_sdma_eng_err_name(char *buf, size_t bsize, unsigned int source)
{
	snprintf(buf, bsize, "SDmaEngErrInt%u", source);
	return buf;
}

/*
 * Return the send context error interrupt name.
 */
static char *is_sendctxt_err_name(char *buf, size_t bsize, unsigned int source)
{
	snprintf(buf, bsize, "SendCtxtErrInt%u", source);
	return buf;
}

static const char *various_names[] = {
	"PcbInt",
	"GpioAssertInt",
	"Qsfp1Int",
	"Qsfp2Int",
};

/*
 * Return the various interrupt name.
 */
static char *is_various_name(char *buf, size_t bsize, unsigned int source)
{
	if (source < ARRAY_SIZE(various_names))
		strncpy(buf, various_names[source], bsize);
	else
		snprintf(buf, bsize, "Reserved%u", source+WFR_IS_VARIOUS_START);
	return buf;
}

/*
 * Return the DC interrupt name.
 */
static char *is_dc_name(char *buf, size_t bsize, unsigned int source)
{
	static const char *dc_int_names[] = {
		"common",
		"lcb",
		"8051",
		"lbm"	/* local blcok merge */
	};

	if (source < ARRAY_SIZE(dc_int_names))
		snprintf(buf, bsize, "dc_%s_int", dc_int_names[source]);
	else
		snprintf(buf, bsize, "DCInt%u", source);
	return buf;
}

static const char *sdma_int_names[] = {
	"SDmaInt",
	"SdmaIdleInt",
	"SdmaProgressInt",
};

/*
 * Return the SDMA engine interrupt name.
 */
static char *is_sdma_eng_name(char *buf, size_t bsize, unsigned int source)
{
	/* what interrupt */
	unsigned int what  = source / WFR_TXE_NUM_SDMA_ENGINES;
	/* which engine */
	unsigned int which = source % WFR_TXE_NUM_SDMA_ENGINES;

	if (likely(what < 3))
		snprintf(buf, bsize, "%s%u", sdma_int_names[what], which);
	else
		snprintf(buf, bsize, "Invalid SDMA interrupt %u", source);
	return buf;
}

/*
 * Return the receive available interrupt name.
 */
static char *is_rcv_avail_name(char *buf, size_t bsize, unsigned int source)
{
	snprintf(buf, bsize, "RcvAvailInt%u", source);
	return buf;
}

/*
 * Return the receive urgent interrupt name.
 */
static char *is_rcv_urgent_name(char *buf, size_t bsize, unsigned int source)
{
	snprintf(buf, bsize, "RcvUrgentInt%u", source);
	return buf;
}

/*
 * Return the send credit interrupt name.
 */
static char *is_send_credit_name(char *buf, size_t bsize, unsigned int source)
{
	snprintf(buf, bsize, "SendCreditInt%u", source);
	return buf;
}

/*
 * Return the reserved interrupt name.
 */
static char *is_reserved_name(char *buf, size_t bsize, unsigned int source)
{
	snprintf(buf, bsize, "Reserved%u", source + WFR_IS_RESERVED_START);
	return buf;
}

/*
 * Status is a mask of the 3 possible interrupts for this engine.  It will
 * contain bits _only_ for this SDMA engine.  It will contain at least one
 * bit, it may contain more.
 */
static void handle_sdma_interrupt(struct sdma_engine *per_sdma, u64 status)
{
#ifdef JAG_SDMA_VERBOSITY
dd_dev_err(per_sdma->dd, "JAG SDMA %s:%d %s()\n", __FILE__, __LINE__, __func__);
dd_dev_err(per_sdma->dd, "status: 0x%llx\n", status);
#endif

	qib_sdma_intr(per_sdma->dd->pport);
}

static char *pio_err_status_string(char *buf, int buf_len, u64 flags)
{
	return flag_string(buf, buf_len, flags,
			pio_err_status_flags, ARRAY_SIZE(pio_err_status_flags));
}

static char *egress_err_status_string(char *buf, int buf_len, u64 flags)
{
	return flag_string(buf, buf_len, flags,
		egress_err_status_flags, ARRAY_SIZE(egress_err_status_flags));
}

/* TODO */
static void handle_cce_err(struct hfi_devdata *dd, u32 unused, u64 reg)
{
	dd_dev_info(dd, "CCE Error: 0x%llx (unhandled)\n", reg);
}

/* TODO */
static void handle_rxe_err(struct hfi_devdata *dd, u32 unused, u64 reg)
{
	dd_dev_info(dd, "Receive Error: 0x%llx (unhandled)\n", reg);
}

/* TODO */
static void handle_misc_err(struct hfi_devdata *dd, u32 unused, u64 reg)
{
	dd_dev_info(dd, "Misc Error: 0x%llx (unhandled)\n", reg);
}

/* TODO */
static void handle_pio_err(struct hfi_devdata *dd, u32 unused, u64 reg)
{
	char buf[96];

	dd_dev_info(dd, "PIO Error: %s (unhandled)\n",
		pio_err_status_string(buf, sizeof(buf), reg));
}

/* TODO */
static void handle_sdma_err(struct hfi_devdata *dd, u32 unused, u64 reg)
{
	dd_dev_info(dd, "SDMA Error: 0x%llx (unhandled)\n", reg);
#ifdef JAG_SDMA_VERBOSITY
dd_dev_err(dd, "JAG SDMA %s:%d %s()\n", __FILE__, __LINE__, __func__);
qib_sdma0_dumpstate(dd);
#endif
}

/* TODO */
static void handle_egress_err(struct hfi_devdata *dd, u32 unused, u64 reg)
{
	char buf[96];

	dd_dev_info(dd, "Egress Error: %s (unhandled)\n",
		egress_err_status_string(buf, sizeof(buf), reg));
}

/* TODO */
static void handle_txe_err(struct hfi_devdata *dd, u32 unused, u64 reg)
{
	dd_dev_info(dd, "Send Error: 0x%llx (unhandled)\n", reg);
}

/*
 * The maximum number of times the error clear down will loop before
 * blocking a repeating error.  This value is arbitrary.
 */
#define MAX_CLEAR_COUNT 20

/*
 * Clear and handle an error register.  All error interrupts are funneled
 * through here to have a central location to correctly handle single-
 * or multi-shot errors.
 *
 * For non per-context registers, call this routine with a context value
 * of 0 so the per-context offset is zero.
 *
 * If the handler loops too many times, assume that something is wrong
 * and can't be fixed, so mask the error bits.
 */
static void interrupt_clear_down(struct hfi_devdata *dd,
				 u32 context,
				 const struct err_reg_info *eri)
{
	u64 lcb_sel = 0;
	u64 reg;
	u32 count;
	int restore_sel = 0;

	if (eri->handler == handle_lcb_err) {
		lcb_sel = read_csr(dd, DC_DC8051_CFG_CSR_ACCESS_SEL);
		if (!(lcb_sel & DC_DC8051_CFG_CSR_ACCESS_SEL_LCB_SMASK)) {
			restore_sel = 1;
			write_csr(dd, DC_DC8051_CFG_CSR_ACCESS_SEL, lcb_sel
				| DC_DC8051_CFG_CSR_ACCESS_SEL_LCB_SMASK);
		}
	}

	/* read in a loop until no more errors are seen */
	count = 0;
	while (1) {
		reg = read_kctxt_csr(dd, context, eri->status);
		if (reg == 0)
			break;
		write_kctxt_csr(dd, context, eri->clear, reg);
		if (likely(eri->handler))
			eri->handler(dd, context, reg);
		count++;
		if (count > MAX_CLEAR_COUNT) {
			u64 mask;

			dd_dev_err(dd, "Repeating %s bits 0x%llx - masking\n",
				eri->desc, reg);
			/*
			 * Read-modify-write so any other masked bits
			 * remain masked.
			 */
			mask = read_kctxt_csr(dd, context, eri->mask);
			mask &= ~reg;
			write_kctxt_csr(dd, context, eri->mask, mask);
			break;
		}
		/*
		 * TODO: This is to work around a bug in simulator
		 * versions before v36.  In versions before v36, the
		 * DC8051_ERR_CLR did not work, making this loop keep
		 * going until the max count was achieved.  We don't
		 * want to mask the DC8051 interrupt, so work around
		 * it by always breaking here.  In the simulator,
		 * we don't need to loop, so the first time through
		 * will handle everything that has come in.
		 */
		if (dd->icode == WFR_ICODE_FUNCTIONAL_SIMULATOR
				&& dd->irev < 36)
			break;
	}

	if (restore_sel)
		write_csr(dd, DC_DC8051_CFG_CSR_ACCESS_SEL, lcb_sel);
}

/*
 * CCE block "misc" interrupt.  Source is < 16.
 */
static void is_misc_err_int(struct hfi_devdata *dd, unsigned int source)
{
	const struct err_reg_info *eri = &misc_errs[source];

	if (eri->handler) {
		interrupt_clear_down(dd, 0, eri);
	} else {
		dd_dev_err(dd, "Unexpected misc interrupt (%u) - reserved\n",
			source);
	}
}

static char *send_err_status_string(char *buf, int buf_len, u64 flags)
{
	return flag_string(buf, buf_len, flags,
			sc_err_status_flags, ARRAY_SIZE(sc_err_status_flags));
}

static char *credit_return_string(char *buf, int buf_len, u64 flags)
{
	return flag_string(buf, buf_len, flags,
			credit_return_flags, ARRAY_SIZE(credit_return_flags));
}

/*
 * Handle a send context error interrupt on the given send context.
 */
static void handle_send_context_err(struct hfi_devdata *dd,
		unsigned int context, u64 err_status)
{
	struct send_context_info *sci;
	struct send_context *sc;
	u64 hw_free;
	char flags[96];
	int write_dropped = 0;
	int packet_dropped = 0;
	int sc_halted = 0;
	int i;

	if (context >= dd->num_send_contexts) {
		dd_dev_err(dd, "unexpected out of range send contexter interrupt %u\n", context);
		return;
	}

	sci = &dd->send_contexts[context];
	sc = sci->sc;

	/* by the time we are called, the credit return should have been
	   updated */
	hw_free = *sc->hw_free;

	dd_dev_info(dd, "%s: sc%d:\n", __func__, sc->context);
	dd_dev_info(dd, "%s:   hw_free: counter 0x%lx, flags: %s\n", __func__,
		(unsigned long)(hw_free & WFR_CR_COUNTER_SMASK)
			>> WFR_CR_COUNTER_SHIFT,
		credit_return_string(flags, sizeof(flags), hw_free));
	dd_dev_info(dd, "%s:   fill 0x%lx, free 0x%lx\n", __func__,
		sc->fill, sc->free);
	dd_dev_info(dd, "%s:   ErrStatus: %s", __func__,
		send_err_status_string(flags, sizeof(flags), err_status));

	/* find out what effect has been had on this context */
	for (i = 0; i < ARRAY_SIZE(sc_err_status_flags); i++) {
		if (err_status & sc_err_status_flags[i].flag) {
			if (sc_err_status_flags[i].extra & SEC_WRITE_DROPPED)
				write_dropped = 1;
			if (sc_err_status_flags[i].extra & SEC_PACKET_DROPPED)
				packet_dropped = 1;
			if (sc_err_status_flags[i].extra & SEC_SC_HALTED)
				sc_halted = 1;
		}
	}

	if (write_dropped) {
		// TODO: do something?
	}

	if (packet_dropped) {
		// TODO: do something?
	}

	if (sc_halted) {
		// TODO: do something  -
		// user context: given an extra indication to user space?
		// 	They can see an error via credit return - maybe?
		//	Question: is the Status bit set for any error or if
		//	the context is halted?
		//	When the user calls in, then restart
		// kernel context: try to restart
		// ack context: same as kernel
	} else {
		// TODO: set a counter?
	}

	// TODO: call sc_release_update()? 
}

/*
 * Send context error interrupt.  Source (context) is < 160.
 *
 * Check and clear the context error status register.
 */
static void is_sendctxt_err_int(struct hfi_devdata *dd, unsigned int source)
{
	interrupt_clear_down(dd, source, &sendctxt_err);
}

static void handle_sdma_eng_err(struct hfi_devdata *dd,
		unsigned int source, u64 status)
{
	struct qib_pportdata *ppd = dd->pport;
	unsigned long flags;

#ifdef JAG_SDMA_VERBOSITY
dd_dev_err(dd, "JAG SDMA %s:%d %s()\n", __FILE__, __LINE__, __func__);
dd_dev_err(dd, "JAG SDMA source: %u\n", source);
dd_dev_err(dd, "JAG SDMA source: 0x%llx\n", (unsigned long long)status);
qib_sdma0_dumpstate(dd);
#endif

	BUG_ON(source != 0);

	spin_lock_irqsave(&ppd->sdma_lock, flags);

	switch (ppd->sdma_state.current_state) {
	case qib_sdma_state_s00_hw_down:
		break;

	case qib_sdma_state_s10_hw_start_up_halt_wait:
		if (status & WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_HALT_ERR_SMASK)
			__qib_sdma_process_event(ppd, qib_sdma_event_e15_hw_started1);
		break;

	case qib_sdma_state_s15_hw_start_up_clean_wait:
		break;

	case qib_sdma_state_s20_idle:
		break;

	case qib_sdma_state_s30_sw_clean_up_wait:
		break;

	case qib_sdma_state_s40_hw_clean_up_wait:
		break;

	case qib_sdma_state_s50_hw_halt_wait:
		break;

	case qib_sdma_state_s99_running:
		break;
	}

	spin_unlock_irqrestore(&ppd->sdma_lock, flags);
}

/*
 * CCE block SDMA error interrupt.  Source is < 16.
 */
static void is_sdma_eng_err_int(struct hfi_devdata *dd, unsigned int source)
{
#ifdef JAG_SDMA_VERBOSITY
dd_dev_err(dd, "JAG SDMA %s:%d %s()\n", __FILE__, __LINE__, __func__);
dd_dev_err(dd, "JAG SDMA source: %u\n", source);
qib_sdma0_dumpstate(dd);
#endif
	interrupt_clear_down(dd, source, &sdma_eng_err);
}

/*
 * CCE block "various" interrupt.  Source is < 8.
 */
static void is_various_int(struct hfi_devdata *dd, unsigned int source)
{
	/* TODO: actually do something */
	printk("%s: int%u - unimplemented\n", __func__ , source);
}

/*
 * Write a response back to a 8051 request.
 */
static void hreq_response(struct hfi_devdata *dd, u8 return_code, u16 rsp_data)
{
	write_csr(dd, DC_DC8051_CFG_EXT_DEV_0,
		DC_DC8051_CFG_EXT_DEV_0_COMPLETED_SMASK
		| (u64)return_code << DC_DC8051_CFG_EXT_DEV_0_RETURN_CODE_SHIFT
		| (u64)rsp_data << DC_DC8051_CFG_EXT_DEV_0_RSP_DATA_SHIFT);
}

/*
 * Handle requests from the 8051.
 */
static void handle_8051_request(struct hfi_devdata *dd)
{
	u64 reg;
	u16 data;
	u8 type;
	
	reg = read_csr(dd, DC_DC8051_CFG_EXT_DEV_1);
	if ((reg & DC_DC8051_CFG_EXT_DEV_1_REQ_NEW_SMASK) == 0)
		return;	/* no request */

	/* zero out COMPLETED so the response is seen */
	write_csr(dd, DC_DC8051_CFG_EXT_DEV_0, 0);

	/* extract request details */
	type = (reg >> DC_DC8051_CFG_EXT_DEV_1_REQ_TYPE_SHIFT)
			& DC_DC8051_CFG_EXT_DEV_1_REQ_TYPE_MASK;
	data = (reg >> DC_DC8051_CFG_EXT_DEV_1_REQ_DATA_SHIFT)
			& DC_DC8051_CFG_EXT_DEV_1_REQ_DATA_MASK;

	switch (type) {
	case WFR_HREQ_LOAD_CONFIG:
	case WFR_HREQ_SAVE_CONFIG:
	case WFR_HREQ_READ_CONFIG:
	case WFR_HREQ_SET_TX_EQ_ABS:
	case WFR_HREQ_SET_TX_EQ_REL:
	case WFR_HREQ_ENABLE:
		dd_dev_info(dd, "8051 request: request 0x%x not supported\n",
			type);
		hreq_response(dd, WFR_HREQ_NOT_SUPPORTED, 0);
		break;

	case WFR_HREQ_CONFIG_DONE:
		hreq_response(dd, WFR_HREQ_SUCCESS, 0);
		break;

	case WFR_HREQ_INTERFACE_TEST:
		hreq_response(dd, WFR_HREQ_SUCCESS, data);
		break;

	default:
		dd_dev_err(dd, "8051 request: unknown request 0x%x\n", type);
		hreq_response(dd, WFR_HREQ_NOT_SUPPORTED, 0);
		break;
	};
}

void write_global_credit(struct hfi_devdata *dd, u8 vau, u16 total, u16 shared)
{
	write_csr(dd, WFR_SEND_CM_GLOBAL_CREDIT,
/* TODO: HAS 0.76 changed names and adds a field */
#ifdef WFR_SEND_CM_GLOBAL_CREDIT_SHARED_LIMIT_SHIFT
		((u64)total
			<< WFR_SEND_CM_GLOBAL_CREDIT_TOTAL_CREDIT_LIMIT_SHIFT)
		| ((u64)shared
			<< WFR_SEND_CM_GLOBAL_CREDIT_SHARED_LIMIT_SHIFT)
#else
		((u64)total
			<< WFR_SEND_CM_GLOBAL_CREDIT_GLOBAL_LIMIT_SHIFT)
#endif
		| ((u64)vau << WFR_SEND_CM_GLOBAL_CREDIT_AU_SHIFT));
}

/*
 * Set up initial VL15 credits of the remote.  Assumes the rest of
 * the CM credit registers are zero from a previous global or credit reset .
 */
void set_up_vl15(struct hfi_devdata *dd, u8 vau, u16 vl15buf)
{
	/* leave shared count at zero for both global and VL15 */
	write_global_credit(dd, vau, vl15buf, 0);
	write_csr(dd, WFR_SEND_CM_CREDIT_VL15, (u64)vl15buf
		    << WFR_SEND_CM_CREDIT_VL15_DEDICATED_LIMIT_VL_SHIFT);
}

/*
 * Zero all credit details from the previous connection and
 * reset the CM manager's internal counters.
 */
void reset_link_credits(struct hfi_devdata *dd)
{
	int i;

	/* remove all previous VL credit limits */
	for (i = 0; i < WFR_TXE_NUM_DATA_VL; i++)
		write_csr(dd, WFR_SEND_CM_CREDIT_VL + (8*i), 0);
	write_csr(dd, WFR_SEND_CM_CREDIT_VL15, 0);
	write_global_credit(dd, 0, 0, 0);
	/* reset the CM block */
	pio_send_control(dd, PSC_CM_RESET);
}

/* convert a vCU to a CU */
static u32 vcu_to_cu(u8 vcu)
{
	return 1 << vcu;
}

/* convert a CU to a vCU */
static u8 cu_to_vcu(u32 cu)
{
	return ilog2(cu);
}

/* convert a vAU to an AU */
static u32 vau_to_au(u8 vau)
{
	return 8 * (1 << vau);
}

/*
 * Graceful LCB shutdown.  This leaves the LCB FIFOs in reset.
 */
void lcb_shutdown(struct hfi_devdata *dd)
{
	u64 reg, saved_lcb_err_en;

	/* clear lcb run: LCB_CFG_RUN.EN = 0 */
	write_csr(dd, DC_LCB_CFG_RUN, 0);
	/* set tx fifo reset: LCB_CFG_TX_FIFOS_RESET.VAL = 1 */
	write_csr(dd, DC_LCB_CFG_TX_FIFOS_RESET,
		1ull << DC_LCB_CFG_TX_FIFOS_RESET_VAL_SHIFT);
	/* set dcc reset csr: DCC_CFG_RESET.reset_lcb = 1 */
	saved_lcb_err_en = read_csr(dd, DC_LCB_ERR_EN);
	reg = read_csr(dd, DCC_CFG_RESET);
	write_csr(dd, DCC_CFG_RESET,
		reg | 1ull << DCC_CFG_RESET_RESET_LCB_SHIFT);
	(void) read_csr(dd, DCC_CFG_RESET); /* make sure the write completed */
	udelay(1);	/* must hold for the longer of 16cclks or 20ns */
	write_csr(dd, DCC_CFG_RESET, reg);
	write_csr(dd, DC_LCB_ERR_EN, saved_lcb_err_en);
}

/*
 * These LCB adjustments are for the Aurora SerDes core in the FPGA.
 */
void adjust_lcb_for_fpga_serdes(struct hfi_devdata *dd)
{
	u64 rx_radr, tx_radr;

	if (dd->icode != WFR_ICODE_FPGA_EMULATION)
		return;

	if ((dd->irev >> 8) <= 0x12) {
		/* release 0x12 and below */

		/*
		 * LCB_CFG_RX_FIFOS_RADR.RST_VAL = 0x9
		 * LCB_CFG_RX_FIFOS_RADR.OK_TO_JUMP_VAL = 0x9
		 * LCB_CFG_RX_FIFOS_RADR.DO_NOT_JUMP_VAL = 0xa
		 */
		rx_radr =
		      0x9ull << DC_LCB_CFG_RX_FIFOS_RADR_RST_VAL_SHIFT
		    | 0x9ull << DC_LCB_CFG_RX_FIFOS_RADR_OK_TO_JUMP_VAL_SHIFT
		    | 0xaull << DC_LCB_CFG_RX_FIFOS_RADR_DO_NOT_JUMP_VAL_SHIFT;
		/*
		 * LCB_CFG_TX_FIFOS_RADR.ON_REINIT = 0 (default)
		 * LCB_CFG_TX_FIFOS_RADR.RST_VAL = 6
		 */
		tx_radr = 6ull << DC_LCB_CFG_TX_FIFOS_RADR_RST_VAL_SHIFT;
	} else  {
		/* release 0x13 and higher */
		rx_radr =
		      0x8ull << DC_LCB_CFG_RX_FIFOS_RADR_RST_VAL_SHIFT
		    | 0x8ull << DC_LCB_CFG_RX_FIFOS_RADR_OK_TO_JUMP_VAL_SHIFT
		    | 0x9ull << DC_LCB_CFG_RX_FIFOS_RADR_DO_NOT_JUMP_VAL_SHIFT;
		tx_radr = 7ull << DC_LCB_CFG_TX_FIFOS_RADR_RST_VAL_SHIFT;
	}

	write_csr(dd, DC_LCB_CFG_RX_FIFOS_RADR, rx_radr);
	/* LCB_CFG_IGNORE_LOST_RCLK.EN = 1 */
	write_csr(dd, DC_LCB_CFG_IGNORE_LOST_RCLK,
		DC_LCB_CFG_IGNORE_LOST_RCLK_EN_SMASK);
	write_csr(dd, DC_LCB_CFG_TX_FIFOS_RADR, tx_radr);
}

/*
 * Handle a verify capabilities interrupt from the 8051
 */
static void handle_verify_cap(struct hfi_devdata *dd)
{
	u8 power_management;
	u8 continious;
	u8 vcu;
	u8 vau;
	u16 vl15buf;
	u16 flag_bits;
	u16 link_widths;
	u16 crc_mask;
	u16 crc_val;
	u8 crc_sizes;
	struct qib_pportdata *ppd = dd->pport;

	/* give host access to the LCB CSRs */
	write_csr(dd, DC_DC8051_CFG_CSR_ACCESS_SEL,
		read_csr(dd, DC_DC8051_CFG_CSR_ACCESS_SEL)
		| DC_DC8051_CFG_CSR_ACCESS_SEL_LCB_SMASK);
	write_csr(dd, DC_LCB_ERR_EN, ~0ull); /* watch LCB errors */

	lcb_shutdown(dd);
	adjust_lcb_for_fpga_serdes(dd);

	/*
	 * These are now valid:
	 *	remote VerifyCap fields in the general LNI config
	 *	CSR DC8051_STS_REMOTE_GUID
	 *	CSR DC8051_STS_REMOTE_NODE_TYPE
	 *	CSR DC8051_STS_REMOTE_FM_SECURITY
	 */
	read_vc_remote_phy(dd, &power_management, &continious);
	read_vc_remote_fabric(dd, &vau, &vcu, &vl15buf, &crc_sizes);
	read_vc_remote_link_width(dd, &flag_bits, &link_widths);
	dd_dev_info(dd,
		"Peer PHY: power management 0x%x, continous updates 0x%x\n",
		(int)power_management, (int)continious);
	dd_dev_info(dd,
		"Peer Fabric: vAU %d, vCU %d, vl15 credits 0x%x, CRC sizes 0x%x\n",
		(int)vau, (int)vcu, (int)vl15buf, (int)crc_sizes);
	dd_dev_info(dd, "Peer Link Width: flags 0x%x, widths 0x%x\n",
		(u32)flag_bits, (u32)link_widths);
	if (disable_bcc) {
		/*
		 * TODO:
		 * With the BCC disabled, no Verify Cap exchange takes
		 * place, so the above values will all be zero.
		 * For now, use our values as we know that we are
		 * talking back-to-back with another WFR.  If we hook up
		 * with a PRR, we will need to revisit this.  In particular,
		 * vl15buf and vcu, which may be different.  vau is fixed
		 * at 3 for all Gen1 chips.
		 */
		dd_dev_info(dd,
			"Overriding invalid remote details with local data\n");
		vau = dd->vau;
		vl15buf = dd->vl15_init;
		vcu = dd->vcu;
	}
	set_up_vl15(dd, vau, vl15buf);

	/* set up the LCB CRC mode */
	crc_mask = link_crc_mask & crc_sizes;
	/* order is important: use the highest bit in common */
	if (crc_mask & CRC_12B_16B_PER_LANE)
		crc_val = LCB_CRC_12B_16B_PER_LANE;
	else if (crc_mask & CRC_48B)
		crc_val = LCB_CRC_48B;
	else if (crc_mask & CRC_14B)
		crc_val = LCB_CRC_14B;
	else
		crc_val = LCB_CRC_16B;
	write_csr(dd, DC_LCB_CFG_CRC_MODE,
		(u64)crc_val << DC_LCB_CFG_CRC_MODE_TX_VAL_SHIFT);

	/* set up the remote credit return table */
	assign_remote_cm_au_table(dd, vcu);

	/* pull LCB fifos out of reset - all fifo clocks must be stable */
	write_csr(dd, DC_LCB_CFG_TX_FIFOS_RESET, 0);

	/* give 8051 access to the LCB CSRs */
	write_csr(dd, DC_LCB_ERR_EN, 0); /* mask LCB errors */
	write_csr(dd, DC_DC8051_CFG_CSR_ACCESS_SEL,
		read_csr(dd, DC_DC8051_CFG_CSR_ACCESS_SEL)
		& ~DC_DC8051_CFG_CSR_ACCESS_SEL_LCB_SMASK);

	/* tell the 8051 to go to LinkUp */
	set_physical_link_state(dd, WFR_PLS_LINKUP);

	ppd->neighbor_guid =
		cpu_to_be64(read_csr(dd, DC_DC8051_STS_REMOTE_GUID));
	ppd->neighbor_type =
		read_csr(dd, DC_DC8051_STS_REMOTE_NODE_TYPE);
	dd_dev_info(dd, "Neighbor Guid: %llx Neighbor type %d\n",
		be64_to_cpu(ppd->neighbor_guid), ppd->neighbor_type);
}

static char *dcc_err_string(char *buf, int buf_len, u64 flags)
{
	return flag_string(buf, buf_len, flags, dcc_err_flags,
		ARRAY_SIZE(dcc_err_flags));
}

static char *dc8051_err_string(char *buf, int buf_len, u64 flags)
{
	return flag_string(buf, buf_len, flags, dc8051_err_flags,
		ARRAY_SIZE(dc8051_err_flags));
}

static char *dc8051_info_err_string(char *buf, int buf_len, u64 flags)
{
	return flag_string(buf, buf_len, flags, dc8051_info_err_flags,
		ARRAY_SIZE(dc8051_info_err_flags));
}

static char *dc8051_info_host_msg_string(char *buf, int buf_len, u64 flags)
{
	return flag_string(buf, buf_len, flags, dc8051_info_host_msg_flags,
		ARRAY_SIZE(dc8051_info_host_msg_flags));
}

static void handle_8051_interrupt(struct hfi_devdata *dd, u32 unused, u64 reg)
{
	u64 info, err, host_msg;
	char buf[96];

	/* look at the flags */
	if (reg & DC_DC8051_ERR_FLG_SET_BY_8051_SMASK) {
		/* 8051 information set by firmware */
		/* read DC8051_DBG_ERR_INFO_SET_BY_8051 for details */
		info = read_csr(dd, DC_DC8051_DBG_ERR_INFO_SET_BY_8051);
		err = (info >> DC_DC8051_DBG_ERR_INFO_SET_BY_8051_ERROR_SHIFT)
			& DC_DC8051_DBG_ERR_INFO_SET_BY_8051_ERROR_MASK;
		host_msg = (info >>
			DC_DC8051_DBG_ERR_INFO_SET_BY_8051_HOST_MSG_SHIFT)
			& DC_DC8051_DBG_ERR_INFO_SET_BY_8051_HOST_MSG_MASK;

		/*
		 * Handle error flags.
		 */
		if (err) {
			/* TODO: implement 8051 error handling */
			dd_dev_info(dd, "8051 info error: %s (unhandled)\n",
				dc8051_info_err_string(buf, sizeof(buf), err));
		}

		/*
		 * Handle host message flags.
		 */
		if (host_msg & WFR_HOST_REQ_DONE) {
			/*
			 * Presently, the driver does a busy wait for
			 * host requests to complete.  This is only an
			 * informational message that should be
			 * moved to a trace level.
			 * NOTE: The 8051 clears the host message
			 * information _on the next 8051 command_.
			 * Therefore, when linkup is achieved,
			 * this flag will still be set.
			 */
			dd_dev_info(dd, "8051: host request done\n");
			/* clear flag so "uhnandled" message below
			   does not include this */
			host_msg &= ~(u64)WFR_HOST_REQ_DONE;
		}
		if (host_msg & WFR_LINKUP_ACHIEVED) {
			dd_dev_info(dd, "8051: LinkUp achieved\n");
			handle_linkup_change(dd, 1);
			/* clear flag so "uhnandled" message below
			   does not include this */
			host_msg &= ~(u64)WFR_LINKUP_ACHIEVED;
		}
		if (host_msg & WFR_EXT_DEVICE_CFG_REQ) {
			handle_8051_request(dd);
			/* clear flag so "uhnandled" message below
			   does not include this */
			host_msg &= ~(u64)WFR_EXT_DEVICE_CFG_REQ;
		}
		if (host_msg & WFR_VERIFY_CAP_FRAME) {
			handle_verify_cap(dd);
			/* clear flag so "uhnandled" message below
			   does not include this */
			host_msg &= ~(u64)WFR_VERIFY_CAP_FRAME;
		}
		if (host_msg & WFR_LINK_GOING_DOWN) {
			dd_dev_info(dd, "8051: Link down\n");
			handle_linkup_change(dd, 0);
			/* clear flag so "uhnandled" message below
			   does not include this */
			host_msg &= ~(u64)WFR_LINK_GOING_DOWN;
		}
		/* look for unhandled flags */
		if (host_msg) {
			/* TODO: implement all other valid flags here */
			dd_dev_info(dd,
				"8051 info host message: %s (unhandled)\n",
				dc8051_info_host_msg_string(buf, sizeof(buf),
					host_msg));
		}

		/* clear flag so "unhandled" message below does not
		   include this */
		reg &= ~DC_DC8051_ERR_FLG_SET_BY_8051_SMASK;
	}
	if (reg & DC_DC8051_ERR_FLG_LOST_8051_HEART_BEAT_SMASK) {
		/*
		 * Lost the 8051 heartbeat.  If this happens, we
		 * receive constant interrupts about it.  Disable
		 * the interrupt after the first.
		 */
		dd_dev_err(dd, "Lost 8051 heartbeat\n");
		write_csr(dd, DC_DC8051_ERR_EN,
			read_csr(dd, DC_DC8051_ERR_EN)
			  & ~DC_DC8051_ERR_EN_LOST_8051_HEART_BEAT_SMASK);

		/* clear flag so "unhandled" message below does not
		   include this */
		reg &= ~DC_DC8051_ERR_FLG_LOST_8051_HEART_BEAT_SMASK;
	}
	if (reg) {
		/* TODO: implement all other flags here */
		dd_dev_info(dd, "%s: 8051 Error: %s (unhandled)\n", __func__,
			dc8051_err_string(buf, sizeof(buf), reg));
	}
}

/* TODO */
static void handle_dcc_err(struct hfi_devdata *dd, u32 unused, u64 reg)
{
	u64 info, hdr0, hdr1;
	char *extra;
	char buf[96];

	if (reg & DCC_ERR_FLG_FMCONFIG_ERR_SMASK) {
		info = read_csr(dd, DCC_ERR_INFO_FMCONFIG);
		switch (info) {
		case 0:
			extra = "BadHeadDist: Distance violation between "
				"two head flits";
			break;
		case 1:
			extra = "BadTailDist: Distance violation between "
				"two tail flits";
			break;
		case 2:
			extra = "BadCtrlDist: Dstance violation between "
				"two credit control flits";
			break;
		case 3:
			extra = "BadCrdAck: Credits return for unsupported VL";
			break;
		case 4:
			extra = "UnsupportedVLMarker: Received VL Marker";
			break;
		case 5:
			extra = "BadPreempt: Exceeded the preemtion nesting "
				"level";
			break;
		case 6:
			extra = "BadControlFlit: Received unsupport control "
				"flit";
			break;
		case 8:
			extra = "UnsupportedVL: Recevied VL that was not "
				"configured";
			break;
		default:
			snprintf(buf, sizeof(buf), "reserved%lld", info);
			extra = buf;
			break;
		}
		/* just report this */
		dd_dev_info(dd, "DCC Error: fmconfig error: %s\n",
			extra);

		/* strip so we don't see in the generic unhandled */
		reg &= ~DCC_ERR_FLG_FMCONFIG_ERR_SMASK;
	}

	if (reg & DCC_ERR_FLG_RCVPORT_ERR_SMASK) {
		info = read_csr(dd, DCC_ERR_INFO_PORTRCV);
		hdr0 = read_csr(dd, DCC_ERR_INFO_PORTRCV_HDR0);
		hdr1 = read_csr(dd, DCC_ERR_INFO_PORTRCV_HDR1);
		switch (info) {
		case 1:
			extra = "BadPktLen: Illegal PktLen";
			break;
		case 2:
			extra = "PktLenTooLong: Packet longer than PktLen";
			break;
		case 3:
			extra = "PktLenTooShort: Packet shorterthan PktLen";
			break;
		case 4:
			extra = "BadSLID: Illegal SLID (0, using multicast "
				"as SLID, does not include security "
				"validation of SLID)";
			break;
		case 5:
			extra = "BadDLID: Illegal DLID (0, doesn't match HFI)";
			break;
		case 6:
			extra = "BadL2: Illegal L2 opcode";
			break;
		case 7:
			extra = "BadSC: Unsupported SC";
			break;
		case 9:
			extra = "BadRC: Illegal RC";
			break;
		case 11:
			extra = "PreemptError: Preempting with same VL";
			break;
		case 12:
			extra = "PreemptVL15: Preempting a VL15 packet";
			break;
		case 13:
			extra = "BadVLMarker: VL Marker for an unpreempted VL";
			break;
		default:
			snprintf(buf, sizeof(buf), "reserved%lld", info);
			extra = buf;
			break;
		}
		/* just report this */
		dd_dev_info(dd, "DCC Error: PortRcv error: %s\n",
			extra);
		dd_dev_info(dd, "           hdr0 0x%llx, hdr1 0x%llx\n",
			hdr0, hdr1);

		/* strip so we don't see in the generic unhandled */
		reg &= ~DCC_ERR_FLG_RCVPORT_ERR_SMASK;
	}

	if (reg)
		dd_dev_info(dd, "DCC Error: %s (unhandled)\n",
			dcc_err_string(buf, sizeof(buf), reg));
}

/* TODO */
static void handle_lcb_err(struct hfi_devdata *dd, u32 unused, u64 reg)
{
	dd_dev_info(dd, "LCB Error: 0x%llx (unhandled)\n", reg);
}

/*
 * CCE block DC interrupt.  Source is < 8.
 */
static void is_dc_int(struct hfi_devdata *dd, unsigned int source)
{
	const struct err_reg_info *eri = &dc_errs[source];

	if (eri->handler) {
		interrupt_clear_down(dd, 0, eri);
	} else if (source == 3 /* dc_lbm_int */) {
		/*
		 * This indicates that a parity error has occurred on the
		 * address/control lines presented to the LBM.  The error
		 * is a single pulse, there is no associated error flag,
		 * and it is not maskable.  This is because if a parity
		 * error occurs on the request the request is dropped.
		 * This should never occur, but it is nice to know if it
		 * ever does.
		 */
		dd_dev_err(dd, "Parity error in DC LBM block\n");
	} else {
		dd_dev_err(dd, "Invalid DC interrupt %u\n", source);
	}
}

/*
 * TX block send credit interrupt.  Source is < 160.
 */
static void is_send_credit_int(struct hfi_devdata *dd, unsigned int source)
{
	if (unlikely(source >= dd->num_send_contexts)) {
		dd_dev_err(dd, "unexpected out of range send context credit return interrupt %u\n", source);
		return;
	}
	sc_release_update(dd->send_contexts[source].sc);
}

/*
 * TX block SDMA interrupt.  Source is < 48.
 *
 * SDMA interrupts are grouped by type:
 *
 *	 0 -  N-1 = SDma
 *	 N - 2N-1 = SDmaProgress
 *	2N - 3N-1 = SDmaIdle
 */
static void is_sdma_eng_int(struct hfi_devdata *dd, unsigned int source)
{
	/* what interrupt */
	unsigned int what  = source / WFR_TXE_NUM_SDMA_ENGINES;
	/* which engine */
	unsigned int which = source % WFR_TXE_NUM_SDMA_ENGINES;

#ifdef JAG_SDMA_VERBOSITY
dd_dev_err(dd, "JAG SDMA %s:%d %s()\n", __FILE__, __LINE__, __func__);
qib_sdma0_dumpstate(dd);
#endif

	if (likely(what < 3 && which < dd->num_sdma)) {
		handle_sdma_interrupt(&dd->per_sdma[which], 1ull << what);
	} else {
		/* should not happen */
		dd_dev_err(dd, "Invalid SDMA interrupt 0x%x\n", source);
	}
}

/*
 * RX block receive available interrupt.  Source is < 160.
 */
static void is_rcv_avail_int(struct hfi_devdata *dd, unsigned int source)
{
	struct qib_ctxtdata *rcd;
	char *err_detail;

	if (likely(source < dd->num_rcv_contexts)) {
		rcd = dd->rcd[source];
		if (rcd) {
			handle_receive_interrupt(rcd);
			return;	/* OK */
		}
		/* received an interrupt, but no rcd */
		err_detail = "dataless";
	} else {
		/* received an interrupt, but are not using that context */
		err_detail = "out of range";
	}
	dd_dev_err(dd, "unexpected %s receive available context interrupt %u\n", err_detail, source);
}

/*
 * RX block receive urgent interrupt.  Source is < 160.
 */
static void is_rcv_urgent_int(struct hfi_devdata *dd, unsigned int source)
{
	struct qib_ctxtdata *rcd;
	char *err_detail;

	if (likely(source < dd->num_rcv_contexts)) {
		rcd = dd->rcd[source];
		if (rcd) {
			/* TODO: implement a handler */
			dd_dev_err(dd, "%s: urgent context %u interrupt - unimplemented\n", __func__, rcd->ctxt);
			return;	/* OK */
		}
		/* received an interrupt, but no rcd */
		err_detail = "dataless";
	} else {
		/* received an interrupt, but are not using that context */
		err_detail = "out of range";
	}
	dd_dev_err(dd, "unexpected %s receive urgent context interrupt %u\n", err_detail, source);
}

/*
 * Reserved range interrupt.  Should not be called in normal operation.
 */
static void is_reserved_int(struct hfi_devdata *dd, unsigned int source)
{
	char name[64];

	dd_dev_err(dd, "unexpected %s interrupt\n",
				is_reserved_name(name, sizeof(name), source));
}

/*
 * Interrupt source table.
 *
 * Each entry is an interrupt source "type".  It is ordered by increasing
 * number.
 */
struct is_table {
	int start;	 /* interrupt source type start */
	int end;	 /* interrupt source type end */
	/* routine that returns the name of the interrupt source */
	char *(*is_name)(char *name, size_t size, unsigned int source);
	/* routine to call when receiving an interrupt */
	void (*is_int)(struct hfi_devdata *dd, unsigned int source);
};

static const struct is_table is_table[] = {
/* start		     end
				name func		interrupt func */
{ WFR_IS_GENERAL_ERR_START,  WFR_IS_GENERAL_ERR_END,
				is_misc_err_name,	is_misc_err_int },
{ WFR_IS_SDMAENG_ERR_START,  WFR_IS_SDMAENG_ERR_END,
				is_sdma_eng_err_name,	is_sdma_eng_err_int },
{ WFR_IS_SENDCTXT_ERR_START, WFR_IS_SENDCTXT_ERR_END,
				is_sendctxt_err_name,	is_sendctxt_err_int },
{ WFR_IS_SDMA_START,	     WFR_IS_SDMA_END,
				is_sdma_eng_name,	is_sdma_eng_int },
{ WFR_IS_VARIOUS_START,	     WFR_IS_VARIOUS_END,
				is_various_name,	is_various_int },
{ WFR_IS_DC_START,	     WFR_IS_DC_END,
				is_dc_name,		is_dc_int },
{ WFR_IS_RCVAVAIL_START,     WFR_IS_RCVAVAIL_END,
				is_rcv_avail_name,	is_rcv_avail_int },
{ WFR_IS_RCVURGENT_START,    WFR_IS_RCVURGENT_END,
				is_rcv_urgent_name,	is_rcv_urgent_int },
{ WFR_IS_SENDCREDIT_START,   WFR_IS_SENDCREDIT_END,
				is_send_credit_name,	is_send_credit_int},
{ WFR_IS_RESERVED_START,     WFR_IS_RESERVED_END,
				is_reserved_name,	is_reserved_int},
};

/*
 * Interrupt source name - return the buffer with the text name of the
 * interrupt source.  Source is a bit index into an array of 64-bit integers.
 */
static char *is_name(char *buf, size_t bsize, unsigned int source)
{
	const struct is_table *entry;

	/* avoids a double compare by walking the table in-order */
	for (entry = &is_table[0]; entry->is_name; entry++) {
		if (source < entry->end)
			return entry->is_name(buf, bsize, source-entry->start);
	}
	/* fell off the end */
	snprintf(buf, bsize, "invalid interrupt source %u\n", source);
	return buf;
}

/*
 * Interupt source interrupt - called when the given source has an interrupt.
 * Source is a bit index into an array of 64-bit integers.
 */
static void is_interrupt(struct hfi_devdata *dd, unsigned int source)
{
	const struct is_table *entry;

	/* avoids a double compare by walking the table in-order */
	for (entry = &is_table[0]; entry->is_name; entry++) {
		if (source < entry->end) {
			entry->is_int(dd, source-entry->start);
			return;
		}
	}
	/* fell off the end */
	dd_dev_err(dd, "invalid interrupt source %u\n", source);
}

/*
 * General interrupt handler.  This is able to correctly handle
 * all interrupts in case INTx is used.
 */
static irqreturn_t general_interrupt(int irq, void *data)
{
	struct hfi_devdata *dd = data;
	u64 regs[WFR_CCE_NUM_INT_CSRS];
	u32 bit;
	int i;

	this_cpu_inc(*dd->int_counter);

	/* phase 1: scan and clear all handled interrupts */
	for (i = 0; i < WFR_CCE_NUM_INT_CSRS; i++) {
		if (dd->gi_mask[i] == 0) {
			regs[i] = 0;	/* used later */
			continue;
		}
		regs[i] = read_csr(dd, WFR_CCE_INT_STATUS + (8 * i)) &
				dd->gi_mask[i];
		/* only clear if anything is set */
		if (regs[i])
			write_csr(dd, WFR_CCE_INT_CLEAR + (8 * i), regs[i]);
	}

	/* phase 2: call the apropriate handler */
	for_each_set_bit(bit, (unsigned long *)&regs[0],
						WFR_CCE_NUM_INT_CSRS*64) {
#if 0
		/* TODO: the print is temporary */
		char buf[64];
		printk(DRIVER_NAME"%d: interrupt %d: %s\n", dd->unit, bit,
			is_name(buf, sizeof(buf), bit));
		/* TODO implement trace down in is_interrupt() */
#endif
		is_interrupt(dd, bit);
	}

	return IRQ_HANDLED;
}

static irqreturn_t sdma_interrupt(int irq, void *data)
{
	struct sdma_engine *per_sdma = data;
	struct hfi_devdata *dd = per_sdma->dd;
	u64 status;

#ifdef JAG_SDMA_VERBOSITY
dd_dev_err(dd, "JAG SDMA %s:%d %s()\n", __FILE__, __LINE__, __func__);
qib_sdma0_dumpstate(dd);
#endif

	this_cpu_inc(*dd->int_counter);

	status = read_csr(dd,
			WFR_CCE_INT_STATUS + (8*(WFR_IS_SDMA_START/64)))
			& per_sdma->imask;
	if (likely(status)) {
		/* clear the interrupt(s) */
		write_csr(dd,
			WFR_CCE_INT_CLEAR + (8*(WFR_IS_SDMA_START/64)),
			status);

		/* handle the interrupt(s) */
		handle_sdma_interrupt(per_sdma, status);
	} else {
		dd_dev_err(dd, "SDMA engine %d interrupt, but no status bits set\n", per_sdma->which);
	}

	return IRQ_HANDLED;
}

/*
 * NOTE: this routine expects to be on its own MSI-X interrupt.  If
 * multiple receive contexts share the same MSI-X interupt, then this
 * routine must check for who received it.
 */
static irqreturn_t receive_context_interrupt(int irq, void *data)
{
	struct qib_ctxtdata *rcd = data;
	struct hfi_devdata *dd = rcd->dd;

	trace_hfi_receive_interrupt(dd, rcd->ctxt);
	this_cpu_inc(*dd->int_counter);

	/* clear the interrupt */
	write_csr(rcd->dd, WFR_CCE_INT_CLEAR + (8*rcd->ireg), rcd->imask);

	/* handle the interrupt */
	handle_receive_interrupt(rcd);

	return IRQ_HANDLED;
}

/* ========================================================================= */

static void sdma_sendctrl(struct qib_pportdata *ppd, unsigned op)
{
	struct hfi_devdata *dd = ppd->dd;
	u64 set_senddmactrl = 0;
	u64 clr_senddmactrl = 0;

#ifdef JAG_SDMA_VERBOSITY
dd_dev_err(dd, "JAG SDMA %s:%d %s()\n", __FILE__, __LINE__, __func__);
dd_dev_err(dd, "JAG SDMA senddmactrl E=%d I=%d H=%d C=%d\n",
	(op & QIB_SDMA_SENDCTRL_OP_ENABLE) ? 1 : 0,
	(op & QIB_SDMA_SENDCTRL_OP_INTENABLE) ? 1 : 0,
	(op & QIB_SDMA_SENDCTRL_OP_HALT) ? 1 : 0,
	(op & QIB_SDMA_SENDCTRL_OP_CLEANUP) ? 1 : 0);
#endif

	if (op & QIB_SDMA_SENDCTRL_OP_ENABLE)
		set_senddmactrl |= WFR_SEND_DMA_CTRL_SDMA_ENABLE_SMASK;
	else
		clr_senddmactrl |= WFR_SEND_DMA_CTRL_SDMA_ENABLE_SMASK;

	if (op & QIB_SDMA_SENDCTRL_OP_INTENABLE)
		set_senddmactrl |= WFR_SEND_DMA_CTRL_SDMA_INT_ENABLE_SMASK;
	else
		clr_senddmactrl |= WFR_SEND_DMA_CTRL_SDMA_INT_ENABLE_SMASK;

	if (op & QIB_SDMA_SENDCTRL_OP_HALT)
		set_senddmactrl |= WFR_SEND_DMA_CTRL_SDMA_HALT_SMASK;
	else
		clr_senddmactrl |= WFR_SEND_DMA_CTRL_SDMA_HALT_SMASK;

	/* JAG TODO: OP_DRAIN */

	spin_lock(&dd->per_sdma[0].senddmactrl_lock);

	/* JAG TODO: OP_DRAIN */

	dd->per_sdma[0].p_senddmactrl |= set_senddmactrl;
	dd->per_sdma[0].p_senddmactrl &= ~clr_senddmactrl;

	if (op & QIB_SDMA_SENDCTRL_OP_CLEANUP)
		write_kctxt_csr(dd, 0, WFR_SEND_DMA_CTRL,
			dd->per_sdma[0].p_senddmactrl | WFR_SEND_DMA_CTRL_SDMA_CLEANUP_SMASK);
	else
		write_kctxt_csr(dd, 0, WFR_SEND_DMA_CTRL, dd->per_sdma[0].p_senddmactrl);
	/* JAG XXX: do we have kr_scratch need/equivalent */

	/* JAG TODO: OP_DRAIN */

	spin_unlock(&dd->per_sdma[0].senddmactrl_lock);

	/* JAG TODO: OP_DRAIN */
#ifdef JAG_SDMA_VERBOSITY
qib_sdma0_dumpstate(dd);
#endif
}

static void sdma_hw_clean_up(struct qib_pportdata *ppd)
{

#ifdef JAG_SDMA_VERBOSITY
dd_dev_err(ppd->dd, "JAG SDMA %s:%d %s()\n", __FILE__, __LINE__, __func__);
#endif

	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);

#ifdef JAG_SDMA_VERBOSITY
qib_sdma0_dumpstate(ppd->dd);
#endif
}

static void sdma_setlengen(struct qib_pportdata *ppd)
{
#ifdef JAG_SDMA_VERBOSITY
dd_dev_err(ppd->dd, "JAG SDMA %s:%d %s()\n", __FILE__, __LINE__, __func__);
#endif

	/*
	 * Set SendDmaLenGen and clear-then-set the MSB of the generation
	 * count to enable generation checking and load the internal
	 * generation counter.
	 */
	write_kctxt_csr(ppd->dd, 0, WFR_SEND_DMA_LEN_GEN,
		(ppd->sdma_descq_cnt/64) << WFR_SEND_DMA_LEN_GEN_LENGTH_SHIFT
	);
	write_kctxt_csr(ppd->dd, 0, WFR_SEND_DMA_LEN_GEN,
		((ppd->sdma_descq_cnt/64) << WFR_SEND_DMA_LEN_GEN_LENGTH_SHIFT)
		| (4ULL << WFR_SEND_DMA_LEN_GEN_GENERATION_SHIFT)
	);
}

static void sdma_update_tail(struct qib_pportdata *ppd, u16 tail)
{
#ifdef JAG_SDMA_VERBOSITY
dd_dev_err(ppd->dd, "JAG SDMA %s:%d %s()\n", __FILE__, __LINE__, __func__);
dd_dev_err(ppd->dd, "tail: 0x%x --> 0x%x\n", ppd->sdma_descq_tail, tail);

do {
u16 i, start, end;
start = ppd->sdma_descq_tail;
end = tail;
if (start < end) {
/**/for (i = start; i < end; ++i) {
/**//**/dd_dev_err(ppd->dd, "desc[%d] = %016llx %016llx\n",
/**//**//**/i,
/**//**//**/le64_to_cpu(ppd->sdma_descq[i].qw[0]),
/**//**//**/le64_to_cpu(ppd->sdma_descq[i].qw[1])
/**//**/);
/**/}
} else if (end < start) {
/**/for (i = start; i < ppd->sdma_descq_cnt; ++i) {
/**//**/dd_dev_err(ppd->dd, "desc[%d] = %016llx %016llx\n",
/**//**//**/i,
/**//**//**/le64_to_cpu(ppd->sdma_descq[i].qw[0]),
/**//**//**/le64_to_cpu(ppd->sdma_descq[i].qw[1])
/**//**/);
/**/}
/**/for (i = 0; i < end; ++i) {
/**//**/dd_dev_err(ppd->dd, "desc[%d] = %016llx %016llx\n",
/**//**//**/i,
/**//**//**/le64_to_cpu(ppd->sdma_descq[i].qw[0]),
/**//**//**/le64_to_cpu(ppd->sdma_descq[i].qw[1])
/**//**/);
/**/}
} else {
/**/dd_dev_err(ppd->dd, "Empty!???\n");
}} while (0);
#endif

	/* Commit writes to memory and advance the tail on the chip */
	wmb();
	ppd->sdma_descq_tail = tail;
	write_kctxt_csr(ppd->dd, 0, WFR_SEND_DMA_TAIL, tail);
}

static void sdma_hw_start_up(struct qib_pportdata *ppd)
{
#ifdef JAG_SDMA_VERBOSITY
dd_dev_err(ppd->dd, "JAG SDMA %s:%d %s()\n", __FILE__, __LINE__, __func__);
#endif

/* JAG SDMA keep until fixed in simics */
write_kctxt_csr(ppd->dd, 0, WFR_SEND_DMA_MEMORY, DEFAULT_WFR_SEND_DMA_MEMORY);
write_kctxt_csr(ppd->dd, 0, WFR_SEND_DMA_BASE_ADDR, ppd->sdma_descq_phys);

	sdma_setlengen(ppd);
	sdma_update_tail(ppd, 0); /* Set SendDmaTail */
	ppd->sdma_head_dma[0] = 0;
	//sdma_sendctrl(ppd, ppd->sdma_state.current_op | QIB_SDMA_SENDCTRL_OP_CLEANUP);
}

static void set_armlaunch(struct hfi_devdata *dd, u32 enable)
{
	if (print_unimplemented)
		dd_dev_info(dd, "%s: not implemented\n", __func__);
}

static u32 read_physical_state(struct hfi_devdata *dd)
{
	u64 reg;
	
	reg = read_csr(dd, DC_DC8051_STS_CUR_STATE);
	return (reg >> DC_DC8051_STS_CUR_STATE_PORT_SHIFT)
				& DC_DC8051_STS_CUR_STATE_PORT_MASK;
}

static u32 __read_logical_state(struct hfi_devdata *dd)
{
	u64 reg;

	reg = read_csr(dd, DCC_CFG_PORT_CONFIG);
	return (reg >> DCC_CFG_PORT_CONFIG_LINK_STATE_SHIFT)
				& DCC_CFG_PORT_CONFIG_LINK_STATE_MASK;
}

static u32 read_logical_state(struct hfi_devdata *dd)
{
	if (sim_easy_linkup) {
		/* return the cached state back */
		switch (dd->pport[0].lstate) {
		default:
			dd_dev_err(dd, "Unknown ib logical state 0x%x, reporting WFR_LSTATE_DOWN\n", dd->pport[9].lstate);
			/* fall through */
		case IB_PORT_DOWN:   return WFR_LSTATE_DOWN;
		case IB_PORT_INIT:   return WFR_LSTATE_INIT;
		case IB_PORT_ARMED:  return WFR_LSTATE_ARMED;
		case IB_PORT_ACTIVE: return WFR_LSTATE_ACTIVE;
		}
	}

	return __read_logical_state(dd);
}

static void set_logical_state(struct hfi_devdata *dd, u32 chip_lstate)
{
	u64 reg;

	if (sim_easy_linkup) {
		/* set the cached state directly */
		dd->pport[0].lstate = chip_to_ib_lstate(dd, chip_lstate);
		return;
	}

	reg = read_csr(dd, DCC_CFG_PORT_CONFIG);
	/* clear current state, set new state */
	reg &= ~DCC_CFG_PORT_CONFIG_LINK_STATE_SMASK;
	reg |= (u64)chip_lstate << DCC_CFG_PORT_CONFIG_LINK_STATE_SHIFT;
	write_csr(dd, DCC_CFG_PORT_CONFIG, reg);
}

static void __print_current_states(struct hfi_devdata *dd, const char *func, const char *extra, u32 physical_state, u32 logical_state)
{
	dd_dev_info(dd, "%s: %s: current physical state 0x%x, logical state 0x%x\n", func, extra, physical_state, logical_state);
}

static void print_current_states(struct hfi_devdata *dd, const char *func, const char *extra)
{
	__print_current_states(dd, func, extra, read_physical_state(dd),
		read_logical_state(dd));
}

/*
 * Returns:
 *	< 0 = Linux error, not able to get access
 * 	> 0 = 8051 command RETURN_CODE
 */
static int do_8051_command(struct hfi_devdata *dd, u32 type, u64 in_data, u64 *out_data)
{
	u64 reg, completed;
	int return_code;
	unsigned long flags;
	unsigned long timeout;

	dd_dev_info(dd, "%s: type %d, data 0x%012llx\n", __func__, type,
		in_data);
	/*
	 * TODO: Do we want to hold the lock for this long?
	 * Alternatives:
	 * - keep busy wait - have other users bounce off
	 */
	spin_lock_irqsave(&dd->dc8051_lock, flags);

	/*
	 * If an 8051 host command timed out previously, then the 8051 is
	 * stuck.  Fail this command immediately.  Handling it this way
	 * allows the driver to properly unload.
	 *
	 * TODO: reset the 8051?
	 * Alternative: Reset the 8051 at timeout time (no need to re-download
	 * firmware).  The concern with this approach is all state, if any,
	 * is lost. OTOH, immediately failing has no chance of recovery.
	 */
	if (dd->dc8051_timed_out) {
		dd_dev_err(dd,
			"Previous 8051 host command timed out, skipping command %u\n",
			type);
		return_code = -ENXIO;
		goto fail;
	}

	/*
	 * If there is no timeout, then the 8051 command interface is
	 * waiting for a command.
	 */

	/*
	 * Follow the HAS and do two writes - the first to stablize
	 * the type and req_data, the second to activate.
	 *
	 * The HW has been DV'ed and is known to work with 1 write.
	 * TODO: In the mean time, the simulator needs at least
	 * 2 writes to operate correctly.  Do that here.
	 */
	reg = ((u64)type & DC_DC8051_CFG_HOST_CMD_0_REQ_TYPE_MASK)
			<< DC_DC8051_CFG_HOST_CMD_0_REQ_TYPE_SHIFT
		| (in_data & DC_DC8051_CFG_HOST_CMD_0_REQ_DATA_MASK)
			<< DC_DC8051_CFG_HOST_CMD_0_REQ_DATA_SHIFT;
	write_csr(dd, DC_DC8051_CFG_HOST_CMD_0, reg);
	reg |= DC_DC8051_CFG_HOST_CMD_0_REQ_NEW_SMASK;
	write_csr(dd, DC_DC8051_CFG_HOST_CMD_0, reg);

	/* wait for completion, alternate: interrupt */
	timeout = jiffies + msecs_to_jiffies(100);
	while (1) {
		reg = read_csr(dd, DC_DC8051_CFG_HOST_CMD_1);
		completed = reg & DC_DC8051_CFG_HOST_CMD_1_COMPLETED_SMASK;
		if (completed)
			break;
		if (time_after(jiffies, timeout)) {
			dd->dc8051_timed_out = 1;
			dd_dev_err(dd, "8051 host command %u timeout\n", type);
			if (out_data)
				*out_data = 0;
			return_code = -ETIMEDOUT;
			goto fail;
		}
		udelay(2);
	}

	if (out_data)
		*out_data = (reg >> DC_DC8051_CFG_HOST_CMD_1_RSP_DATA_SHIFT)
				& DC_DC8051_CFG_HOST_CMD_1_RSP_DATA_MASK;
	return_code = (reg >> DC_DC8051_CFG_HOST_CMD_1_RETURN_CODE_SHIFT)
				& DC_DC8051_CFG_HOST_CMD_1_RETURN_CODE_MASK;
	/*
	 * Clear command for next user.
	 */
	write_csr(dd, DC_DC8051_CFG_HOST_CMD_0, 0);

fail:
	spin_unlock_irqrestore(&dd->dc8051_lock, flags);

	return return_code;
}

static int set_physical_link_state(struct hfi_devdata *dd, u64 state)
{
	if (sim_easy_linkup) {
		/*
		 * Do not change the physical state in easy linkup mode.
		 * But we do need to fake out the HW reaction.
		 */
		switch (state) {
		case WFR_PLS_POLLING:
			/* fake moving to link up */
			dd->pport[0].lstate = IB_PORT_INIT;
			handle_linkup_change(dd, 1);
			break;
		case WFR_PLS_OFFLINE:
			/* going offline will trigger a port down */
			dd->pport[0].lstate = IB_PORT_DOWN;
			break;
		case WFR_PLS_DISABLED:
			/* we can only go to disable from offline, which
			   already hase the lstate set */
			break;
		default:
			dd_dev_err(dd, "%s: unexpected state 0x%llx\n",
				__func__, state);
			break;
		}
		return WFR_HCMD_SUCCESS;
	}

	return do_8051_command(dd, WFR_HCMD_CHANGE_PHY_STATE, state, NULL);
}

static void load_8051_config(struct hfi_devdata *dd, u8 field_id,
				u8 lane_id, u32 config_data)
{
	u64 data;
	int ret;

	data = (u64)field_id << LOAD_DATA_FIELD_ID_SHIFT
		| (u64)lane_id << LOAD_DATA_LANE_ID_SHIFT
		| (u64)config_data << LOAD_DATA_DATA_SHIFT;
	ret = do_8051_command(dd, WFR_HCMD_LOAD_CONFIG_DATA, data, NULL);
	if (ret != WFR_HCMD_SUCCESS) {
		dd_dev_err(dd,
			"load 8051 config: field id %d, lane %d, err %d\n",
			(int)field_id, (int)lane_id, ret);
	}

}

static void read_8051_config(struct hfi_devdata *dd, u8 field_id,
				u8 lane_id, u32 *config_data)
{
	u64 in_data;
	u64 out_data;
	int ret;

	in_data = (u64)field_id << READ_DATA_FIELD_ID_SHIFT
			| (u64)lane_id << READ_DATA_LANE_ID_SHIFT;
	ret = do_8051_command(dd, WFR_HCMD_READ_CONFIG_DATA, in_data,
				&out_data);
	if (ret != WFR_HCMD_SUCCESS) {
		dd_dev_err(dd,
			"read 8051 config: field id %d, lane %d, err %d\n",
			(int)field_id, (int)lane_id, ret);
		out_data = 0;
	}
	*config_data = (out_data >> READ_DATA_DATA_SHIFT) & READ_DATA_DATA_MASK;
}

static void write_vc_local_phy(struct hfi_devdata *dd, u8 power_management,
				u8 continous)
{
	u32 frame;

	frame = continous << CONTINIOUS_REMOTE_UPDATE_SUPPORT_SHIFT
		| power_management << POWER_MANAGEMENT_SHIFT;
	load_8051_config(dd, VERIFY_CAP_LOCAL_PHY, GENERAL_CONFIG, frame);
}

static void write_vc_local_fabric(struct hfi_devdata *dd, u8 vau, u8 vcu,
					u16 vl15buf, u8 crc_sizes)
{
	u32 frame;

	frame = (u32)vau << VAU_SHIFT
		| (u32)vcu << VCU_SHIFT
		| (u32)vl15buf << VL15BUF_SHIFT
		| (u32)crc_sizes << CRC_SIZES_SHIFT;
	load_8051_config(dd, VERIFY_CAP_LOCAL_FABRIC, GENERAL_CONFIG, frame);
}

static void write_vc_local_link_width(struct hfi_devdata *dd,
				u16 flag_bits,
				u16 link_widths)
{
	u32 frame;

	frame = (u32)flag_bits << FLAG_BITS_SHIFT
		| (u32)link_widths << LINK_WIDTH_SHIFT;
	load_8051_config(dd, VERIFY_CAP_LOCAL_LINK_WIDTH, GENERAL_CONFIG,
		frame);
}

static void read_vc_remote_phy(struct hfi_devdata *dd, u8 *power_management,
					u8 *continous)
{
	u32 frame;

	read_8051_config(dd, VERIFY_CAP_REMOTE_PHY, GENERAL_CONFIG, &frame);
	*power_management = (frame >> POWER_MANAGEMENT_SHIFT)
					& POWER_MANAGEMENT_MASK;
	*continous = (frame >> CONTINIOUS_REMOTE_UPDATE_SUPPORT_SHIFT)
					& CONTINIOUS_REMOTE_UPDATE_SUPPORT_MASK;
}

static void read_vc_remote_fabric(struct hfi_devdata *dd, u8 *vau, u8 *vcu,
					u16 *vl15buf, u8 *crc_sizes)
{
	u32 frame;

	read_8051_config(dd, VERIFY_CAP_REMOTE_FABRIC, GENERAL_CONFIG, &frame);
	*vau = (frame >> VAU_SHIFT) & VAU_MASK;
	*vcu = (frame >> VCU_SHIFT) & VCU_MASK;
	*vl15buf = (frame >> VL15BUF_SHIFT) & VL15BUF_MASK;
	*crc_sizes = (frame >> CRC_SIZES_SHIFT) & CRC_SIZES_MASK;
}

static void read_vc_remote_link_width(struct hfi_devdata *dd, u16 *flag_bits,
					u16 *link_widths)
{
	u32 frame;

	read_8051_config(dd, VERIFY_CAP_REMOTE_LINK_WIDTH, GENERAL_CONFIG,
				&frame);
	*flag_bits = (frame >> FLAG_BITS_SHIFT) & FLAG_BITS_MASK;
	*link_widths = (frame >> LINK_WIDTH_SHIFT) & LINK_WIDTH_MASK;
}

int init_loopback(struct hfi_devdata *dd)
{
	u64 reg;
	unsigned long timeout;
	int ret;

	/* give host access to the LCB CSRs */
	write_csr(dd, DC_DC8051_CFG_CSR_ACCESS_SEL,
		read_csr(dd, DC_DC8051_CFG_CSR_ACCESS_SEL)
		| DC_DC8051_CFG_CSR_ACCESS_SEL_LCB_SMASK);
	write_csr(dd, DC_LCB_ERR_EN, ~0ull); /* watch LCB errors */

	lcb_shutdown(dd);

	/*
	 * TODO: Better document loopback values, e.g. define or enum.
	 * This is currently in flux.  Some current values may disappear,
	 * new ones may be added.  Perhaps go with loopback types, e.g.
	 * LCB, SerDes, Cable.
	 */
	/* the simulator can only use LCB loopback v8 */
	if (dd->icode == WFR_ICODE_FUNCTIONAL_SIMULATOR)
		loopback = 8;

	/* configure LCBs in loopback */
	if (loopback == 1 || loopback == 2) {
		/*
		 * Loopback value:
		 * 1 - internal SerDes Loopback, quick linkup
		 * 2 - back-to-back, quick linkup
		 */
		if (loopback == 1) {
			/* this sets the SerDes to internal loopback mode */
			ret = set_physical_link_state(dd,
					WFR_PLS_INTERNAL_SERDES_LOOPBACK);
			if (ret != WFR_HCMD_SUCCESS) {
				dd_dev_err(dd,
					"%s: set phsyical link state to SerDes Loopback failed with return 0x%x\n",
					__func__, ret);
				if (ret >= 0)
					ret = -EINVAL;
				return ret;
			}
		}

		adjust_lcb_for_fpga_serdes(dd);
	} else if (loopback == 8) {	/* LCB loopback for FPGA_P r8 */
		/* LCB_CFG_LOOPBACK.VAL = 2 */
		/* LCB_CFG_LANE_WIDTH.VAL = 0 */
		write_csr(dd, DC_LCB_CFG_LOOPBACK,
			2ull << DC_LCB_CFG_LOOPBACK_VAL_SHIFT);
		write_csr(dd, DC_LCB_CFG_LANE_WIDTH, 0);
	} else if (loopback == 9) {	/* LCB loopback for FPGA_P r9 */
		/* this requires ILP 8051 firmware, or later */
		/*	LCB_CFG_LOOPBACK.VAL = 1 */
		/*	LCB_CFG_LANE_WIDTH.VAL = 0 */
		write_csr(dd, DC_LCB_CFG_LOOPBACK,
			1ull << DC_LCB_CFG_LOOPBACK_VAL_SHIFT);
		write_csr(dd, DC_LCB_CFG_LANE_WIDTH, 0);
	} else {
		dd_dev_err(dd, "%s: Invalid loopback mode %d\n",
			__func__, loopback);
		return -EINVAL;
	}
	/* start the LCBs */
	/*	LCB_CFG_TX_FIFOS_RESET.VAL = 0 */
	write_csr(dd, DC_LCB_CFG_TX_FIFOS_RESET, 0);

	if (loopback == 8) {
		/* LCB_CFG_RUN.EN = 1 */
		write_csr(dd, DC_LCB_CFG_RUN,
			1ull << DC_LCB_CFG_RUN_EN_SHIFT);

		/* watch LCB_STS_LINK_TRANSFER_ACTIVE */
		timeout = jiffies + msecs_to_jiffies(10);
		while (1) {
			reg = read_csr(dd, DC_LCB_STS_LINK_TRANSFER_ACTIVE);
			if (reg)
				break;
			if (time_after(jiffies, timeout)) {
				dd_dev_err(dd,
					"timeout waiting for LINK_TRANSFER_ACTIVE\n");
				return -ETIMEDOUT;
			}
			udelay(2);
		}

		write_csr(dd, DC_LCB_CFG_ALLOW_LINK_UP,
			1ull << DC_LCB_CFG_ALLOW_LINK_UP_VAL_SHIFT);
	}

	write_csr(dd, DC_LCB_ERR_EN, 0); /* mask LCB errors */
	write_csr(dd, DC_DC8051_CFG_CSR_ACCESS_SEL,
			read_csr(dd, DC_DC8051_CFG_CSR_ACCESS_SEL)
				& ~DC_DC8051_CFG_CSR_ACCESS_SEL_LCB_SMASK);

	/*
	 * State "quick" LinkUp request sets the physical link state to
	 * LinkUp without a verify capabilbity sequence.
	 * This state is in simulator v37 and later.
	 */
	ret = set_physical_link_state(dd, WFR_PLS_QUICK_LINKUP);
	if (ret != WFR_HCMD_SUCCESS) {
		dd_dev_err(dd,
			"%s: set phsyical link state to quick LinkUp failed with return 0x%x\n",
			__func__, ret);

		if (ret >= 0)
			ret = -EINVAL;
		return ret;
	}

	/*
	 * In sim v37 and later, the special state move above
	 * sets of the 8051 LinkUp interrupt which calls
	 * handle_link_change().  On hardware, there is no
	 * LinkUp interrupt for the same operation.  Make the
	 * call here, instead.
	 */
	if (dd->icode != WFR_ICODE_FUNCTIONAL_SIMULATOR)
		handle_linkup_change(dd, 1);

	return 0;
}

static int start_polling(struct hfi_devdata *dd)
{
	int ret;

	/* TODO: only start Polling if we have verified that media is
	   present */

	/*
	 * Write link verify capability frames before moving the
	 * link to polling for the first time.
	 */
	/* TODO: Does WFR support Bandwidth control? */
	/* TODO: Does WFR support continuous update? */
	write_vc_local_phy(dd, PWRM_BER_CONTROL, 0);

	write_vc_local_fabric(dd, dd->vau, dd->vcu, dd->vl15_init,
		link_crc_mask);

	/*
	 * TODO: 0x2f00 are undocumented flags co-located with the
	 * local link width capability register.  These flags turn
	 * off some BCC steps during LinkUp, currently necessary
	 * for back-to-back operation (doesn't matter for loopback).
	 */
	write_vc_local_link_width(dd, disable_bcc ? 0x2f00 : 0,
		WFR_SUPPORTED_LINK_WIDTHS);

	ret = set_physical_link_state(dd, WFR_PLS_POLLING);
	if (ret != WFR_HCMD_SUCCESS) {
		dd_dev_err(dd,
			"%s: set phsyical link state to Polling failed with return 0x%x\n",
			__func__, ret);

		if (ret >= 0)
			ret = -EINVAL;
		return ret;
	}
	return 0;
}

void restart_link(unsigned long opaque)
{
	struct qib_pportdata *ppd = (struct qib_pportdata *)opaque;
	int ret;

	if (!ppd->link_enabled)
		return;

	ret = start_polling(ppd->dd);
	if (ret) {
		dd_dev_err(ppd->dd,
			"Unable to restart polling on the link, err %d\n", ret);
		/* for now, just try to restart in 30s */
		mod_timer(&ppd->link_restart_timer, msecs_to_jiffies(30000));
	}
}

static int bringup_serdes(struct qib_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;
	u64 guid, reg;

	/* enable the port */
	/* TODO: 7322: done within rcvmod_lock */
	reg = read_csr(dd, WFR_RCV_CTRL);
	reg |= WFR_RCV_CTRL_RCV_PORT_ENABLE_SMASK;
	/* XXX (Mitko): This should have a better place than here! */
	if (extended_psn)
		reg |= WFR_RCV_CTRL_RCV_EXTENDED_PSN_ENABLE_SMASK;
	write_csr(dd, WFR_RCV_CTRL, reg);

	guid = be64_to_cpu(ppd->guid);
	if (!guid) {
		if (dd->base_guid)
			guid = be64_to_cpu(dd->base_guid) + ppd->port - 1;
		ppd->guid = cpu_to_be64(guid);
	}

	/* the link defaults to enabled */
	ppd->link_enabled = 1;

	if (loopback) {
		dd_dev_info(dd, "Entering loopback mode\n");
		return init_loopback(dd);
	}

	return start_polling(dd);
}

static void quiet_serdes(struct qib_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;
	u64 reg;
	int ret;

	/* link is now disabled */
	ppd->link_enabled = 0;

	ret = set_physical_link_state(dd, WFR_PLS_OFFLINE);
	if (ret == WFR_HCMD_SUCCESS) {
		/*
		 * The DC does not inform us when the link goes down after
		 * we ask it to.  Make an explicit change here
		 */
		handle_linkup_change(dd, 0);
	} else {
		dd_dev_err(dd, "%s: set phsyical link state to Offline.Quiet failed with return 0x%x\n", __func__, ret);
	}

	/* disable the port */
	/* TODO: 7322: done within rcvmod_lock */
	reg = read_csr(dd, WFR_RCV_CTRL);
	reg &= ~WFR_RCV_CTRL_RCV_PORT_ENABLE_SMASK;
	write_csr(dd, WFR_RCV_CTRL, reg);
}

static void setextled(struct qib_pportdata *ppd, u32 on)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
}

static void stop_irq(struct hfi_devdata *dd)
{
	if (print_unimplemented)
		dd_dev_info(dd, "%s: not implemented\n", __func__);
}

static int reset(struct hfi_devdata *dd)
{
	if (print_unimplemented)
		dd_dev_info(dd, "%s: not implemented\n", __func__);
	dd->z_int_counter = hfi_int_counter(dd);
	return 0;
}

int trace_tid;	/* TODO: hook this up with tracing */
static const char *pt_names[] = {
	"expected",
	"eager",
	"invalid"
};

static const char *pt_name(u32 type)
{
	return type >= ARRAY_SIZE(pt_names) ? "unknown" : pt_names[type];
}

/*
 * index is the index into the receive array
 */
static void put_tid(struct hfi_devdata *dd, u32 index,
		    u32 type, unsigned long pa, u16 order)
{
	u64 reg;
	void __iomem *base = (dd->rcvarray_wc ? dd->rcvarray_wc :
			      (dd->kregbase + WFR_RCV_ARRAY));

	if (!(dd->flags & QIB_PRESENT))
		goto done;

	if (type == PT_INVALID) {
		pa = 0;
	} else if (type > PT_INVALID) {
		dd_dev_err(dd,
			"unexpeced receive array type %u for index %u, not handled\n",
			type, index);
		goto done;
	}

	if (trace_tid)
		dd_dev_info(dd, "%s: type %s, index 0x%x, pa 0x%lx, bsize 0x%lx\n",
			__func__, pt_name(type), index, pa,
			(unsigned long)order);

#define RT_ADDR_SHIFT 12	/* 4KB kernel address boundary */
	reg = WFR_RCV_ARRAY_RT_WRITE_ENABLE_SMASK
		| (u64)order << WFR_RCV_ARRAY_RT_BUF_SIZE_SHIFT
		| ((pa >> RT_ADDR_SHIFT) & WFR_RCV_ARRAY_RT_ADDR_MASK)
					<< WFR_RCV_ARRAY_RT_ADDR_SHIFT;
	writeq(cpu_to_le64(reg), base + (index * 8));

	if (type == PT_EAGER)
		/*
		 * Eager entries are written one-by-one so we have to push them
		 * after we write the entry.
		 */
		qib_flush_wc();
done:
	return;
}

static void clear_tids(struct qib_ctxtdata *rcd)
{
	struct hfi_devdata *dd = rcd->dd;
	u32 i;

	/* TODO: this could be optimized */
	for (i = rcd->eager_base; i < rcd->eager_base + rcd->eager_count; i++)
		put_tid(dd, i, PT_INVALID, 0, 0);

	for (i = rcd->expected_base;
			i < rcd->expected_base + rcd->expected_count; i++)
		put_tid(dd, i, PT_INVALID, 0, 0);
}

static int get_base_info(struct qib_ctxtdata *rcd,
				  struct hfi_base_info *kinfo)
{
	if (rcd->dd->flags & QIB_NODMA_RTAIL)
		kinfo->runtime_flags |= HFI_RUNTIME_NODMA_RTAIL;
	if (extended_psn)
		kinfo->runtime_flags |= HFI_RUNTIME_EXTENDED_PSN;
	return 0;
}

static struct qib_message_header *get_msgheader(
				struct hfi_devdata *dd, __le32 *rhf_addr)
{
	u32 offset = rhf_hdrq_offset(rhf_addr);

	return (struct qib_message_header *)
		(rhf_addr - dd->rhf_offset + offset);
}

static const char *ib_cfg_name_strings[] = {
	"QIB_IB_CFG_LIDLMC",
	"unused1",
	"QIB_IB_CFG_LWID_ENB",
	"QIB_IB_CFG_LWID",
	"QIB_IB_CFG_SPD_ENB",
	"QIB_IB_CFG_SPD",
	"QIB_IB_CFG_RXPOL_ENB",
	"QIB_IB_CFG_LREV_ENB",
	"QIB_IB_CFG_LINKLATENCY",
	"QIB_IB_CFG_HRTBT",
	"QIB_IB_CFG_OP_VLS",
	"QIB_IB_CFG_VL_HIGH_CAP",
	"QIB_IB_CFG_VL_LOW_CAP",
	"QIB_IB_CFG_OVERRUN_THRESH",
	"QIB_IB_CFG_PHYERR_THRESH",
	"QIB_IB_CFG_LINKDEFAULT",
	"QIB_IB_CFG_PKEYS",
	"QIB_IB_CFG_MTU",
	"QIB_IB_CFG_LSTATE",
	"QIB_IB_CFG_VL_HIGH_LIMIT",
	"QIB_IB_CFG_PMA_TICKS",
	"QIB_IB_CFG_PORT"
};

static const char *ib_cfg_name(int which)
{
	if (which < 0 || which >= ARRAY_SIZE(ib_cfg_name_strings))
		return "invalid";
	return ib_cfg_name_strings[which];
}

static int get_ib_cfg(struct qib_pportdata *ppd, int which)
{
	struct hfi_devdata *dd = ppd->dd;
	int val = 0;

	switch (which) {
	case QIB_IB_CFG_LWID_ENB: /* allowed Link-width */
		val = ppd->link_width_enabled;
		break;
	case QIB_IB_CFG_LWID: /* currently active Link-width */
		val = ppd->link_width_active;
		break;
	case QIB_IB_CFG_SPD_ENB: /* allowed Link speeds */
		val = ppd->link_speed_enabled;
		break;
	case QIB_IB_CFG_SPD: /* current Link speed */
		val = ppd->link_speed_active;
		break;

	case QIB_IB_CFG_RXPOL_ENB: /* Auto-RX-polarity enable */
	case QIB_IB_CFG_LREV_ENB: /* Auto-Lane-reversal enable */
	case QIB_IB_CFG_LINKLATENCY:
		goto unimplemented;

	case QIB_IB_CFG_OP_VLS:
		val = ppd->vls_operational;
		break;
	case QIB_IB_CFG_VL_HIGH_CAP: /* VL arb high priority table size */
		val = WFR_VL_ARB_HIGH_PRIO_TABLE_SIZE;
		break;
	case QIB_IB_CFG_VL_LOW_CAP: /* VL arb low priority table size */
		val = WFR_VL_ARB_LOW_PRIO_TABLE_SIZE;
		break;
	case QIB_IB_CFG_OVERRUN_THRESH: /* IB overrun threshold */
		val = ppd->overrun_threshold;
		break;
	case QIB_IB_CFG_PHYERR_THRESH: /* IB PHY error threshold */
		val = ppd->phy_error_threshold;
		break;
	case QIB_IB_CFG_LINKDEFAULT: /* IB link default (sleep/poll) */
		val = dd->link_default;
		break;

	case QIB_IB_CFG_HRTBT: /* Heartbeat off/enable/auto */
	case QIB_IB_CFG_PMA_TICKS:
	default:
unimplemented:
		if (print_unimplemented)
			dd_dev_info(dd, "%s: which %s: not implemented\n", __func__, ib_cfg_name(which));
		break;
	}

	return val;
}

/*
 * The largest MAD packet size.
 */
#define MAX_MAD_PACKET 2048

/*
 * Return the maximum header bytes for this device.  This
 * is dependent on the device's receive header entry size.
 * WFR allows this to be set per-receive context, but the
 * driver presently enforces a global value.
 */
u32 lrh_max_header_bytes(struct hfi_devdata *dd)
{
	/*
	 * The maximum non-payload (MTU) bytes in LRH.PktLen are
	 * the Receive Header Entry Size minus the PBC (or RHF) size.
	 * 
	 * dd->rcvhdrentsize is in DW.
	 */
	return (dd->rcvhdrentsize - 2/*PBC/RHF*/) << 2;
}

/*
 * Set Send Length
 * @ppd - per port data
 *
 * Set the MTU by limiting how many DWs may be sent.  The SendLenCheck*
 * registers compare against LRH.PktLen, so use the max bytes included
 * in the LRH.
 *
 * This routine changes all VL values except VL15, which it maintains at
 * the same value.
 */
static void set_send_length(struct qib_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;
	u32 max_hb = lrh_max_header_bytes(dd), maxvlmtu = 0, dcmtu;
	u64 len1 = 0, len2 = (((dd->vld[15].mtu + max_hb) >> 2)
			      & WFR_SEND_LEN_CHECK1_LEN_VL15_MASK) <<
		WFR_SEND_LEN_CHECK1_LEN_VL15_SHIFT;
	int i;

	for (i = 0; i < hfi_num_vls(ppd->vls_supported); i++) {
		if (dd->vld[i].mtu > maxvlmtu)
			maxvlmtu = dd->vld[i].mtu;
		if (i <= 3)
			len1 |= (((dd->vld[i].mtu + max_hb) >> 2)
				 & WFR_SEND_LEN_CHECK0_LEN_VL0_MASK) <<
				((i % 4) * WFR_SEND_LEN_CHECK0_LEN_VL1_SHIFT);
		else
			len2 |= (((dd->vld[i].mtu + max_hb) >> 2)
				 & WFR_SEND_LEN_CHECK1_LEN_VL4_MASK) <<
				((i % 4) * WFR_SEND_LEN_CHECK1_LEN_VL5_SHIFT);
	}
	write_csr(dd, WFR_SEND_LEN_CHECK0, len1);
	write_csr(dd, WFR_SEND_LEN_CHECK1, len2);

	/* Adjust maximum MTU for the port in DC */
	dcmtu = maxvlmtu == 10240 ? DCC_CFG_PORT_MTU_CAP_10240 :
		(ilog2(maxvlmtu >> 8) + 1);
	len1 = read_csr(ppd->dd, DCC_CFG_PORT_CONFIG);
	len1 &= ~DCC_CFG_PORT_CONFIG_MTU_CAP_SMASK;
	len1 |= ((u64)dcmtu & DCC_CFG_PORT_CONFIG_MTU_CAP_MASK) <<
		DCC_CFG_PORT_CONFIG_MTU_CAP_SHIFT;
	write_csr(ppd->dd, DCC_CFG_PORT_CONFIG, len1);
}

static void set_lidlmc(struct qib_pportdata *ppd)
{
	u64 c1 = read_csr(ppd->dd, DCC_CFG_PORT_CONFIG1);
#ifdef DCC_CFG_PORT_CONFIG1_DLID_MASK_SMASK
/* TODO HAS 0.76 - field names changed, same meaning */
	c1 &= ~(DCC_CFG_PORT_CONFIG1_TARGET_DLID_SMASK
		| DCC_CFG_PORT_CONFIG1_DLID_MASK_SMASK);
	c1 |= ((ppd->lid & DCC_CFG_PORT_CONFIG1_TARGET_DLID_MASK)
			<< DCC_CFG_PORT_CONFIG1_TARGET_DLID_SHIFT)|
	      ((ppd->lmc & DCC_CFG_PORT_CONFIG1_DLID_MASK_MASK)
			<< DCC_CFG_PORT_CONFIG1_DLID_MASK_SHIFT);
#else
	c1 &= ~(DCC_CFG_PORT_CONFIG1_LID_SMASK|
	       DCC_CFG_PORT_CONFIG1_LMC_SMASK);
	c1 |= ((ppd->lid & DCC_CFG_PORT_CONFIG1_LID_MASK)
			<< DCC_CFG_PORT_CONFIG1_LID_SHIFT)|
	      ((ppd->lmc & DCC_CFG_PORT_CONFIG1_LMC_MASK)
			<< DCC_CFG_PORT_CONFIG1_LMC_SHIFT);
#endif
	write_csr(ppd->dd, DCC_CFG_PORT_CONFIG1, c1);
}

static int set_link_state(struct qib_pportdata *ppd, u32 val)
{
	struct hfi_devdata *dd = ppd->dd;
	u32 phys_request, logical_request;
	int ret1, ret = 0;

	phys_request = val & 0xffff;
	logical_request = val & 0xffff0000;
	dd_dev_info(dd,
		"%s: logical: %s, physical %s\n",
		__func__,
		logical_request == IB_LINKCMD_ARMED ?
						"LINKCMD_ARMED" :
		logical_request == IB_LINKCMD_ACTIVE ?
						"LINKCMD_ACTIVE" :
		logical_request == IB_LINKCMD_DOWN ?
						"LINKCMD_DOWN" :
						"unknown",
		phys_request == IB_LINKINITCMD_NOP ?
						"LINKINITCMD_NOP" :
		phys_request == IB_LINKINITCMD_POLL ?
						"LINKINITCMD_POLL" :
		phys_request == IB_LINKINITCMD_SLEEP ?
						"LINKINITCMD_SLEEP" :
		phys_request == IB_LINKINITCMD_DISABLE ?
						"LINKINITCMD_DISABLE" :
						"unknwon");
	switch (logical_request) {
	case IB_LINKCMD_ARMED:
		set_logical_state(dd, WFR_LSTATE_ARMED);
		break;
	case IB_LINKCMD_ACTIVE:
		set_logical_state(dd, WFR_LSTATE_ACTIVE);
		break;
	case IB_LINKCMD_DOWN:
		/*
		 * If no physical state change is given,
		 * use the default down state.
		 */
		if (phys_request == IB_LINKINITCMD_NOP)
			phys_request = dd->link_default;
		break;
	default:
		dd_dev_info(dd, "%s: logical request 0x%x: not implemented\n",
			__func__, logical_request);
	}

	switch (phys_request) {
	case IB_LINKINITCMD_NOP:	/* nothing */
		break;
	case IB_LINKINITCMD_POLL:
		/* link is enabled */
		ppd->link_enabled = 1;
		/* must transistion to offline first */
		ret1 = set_physical_link_state(dd, WFR_PLS_OFFLINE);
		if (ret1 != WFR_HCMD_SUCCESS) {
			dd_dev_err(dd,
				"Failed to transition to Offline link state, return 0x%x\n",
				ret1);
			ret = -EINVAL;
			break;
		}
		/*
		 * The above move to physical Offline state will
		 * also move the logical state to Down.  Wait for it.
		 */
		qib_wait_linkstate(ppd, IB_PORT_DOWN, 1000);
		ret1 = set_physical_link_state(dd, WFR_PLS_POLLING);
		if (ret1 != WFR_HCMD_SUCCESS) {
			dd_dev_err(dd,
				"Failed to transition to Polling link state, return 0x%x\n",
				ret1);
			ret = -EINVAL;
			break;
		}
		break;
	case IB_LINKINITCMD_SLEEP:
		/* not valid on WFR */
		dd_dev_err(dd, "Cannot transition Sleep link state\n");
		ret = -EINVAL;
		break;
	case IB_LINKINITCMD_DISABLE:
		/* link is disabled */
		ppd->link_enabled = 0;
		/* must transistion to offline first */
		ret1 = set_physical_link_state(dd, WFR_PLS_OFFLINE);
		if (ret1 != WFR_HCMD_SUCCESS) {
			dd_dev_err(dd,
				"Failed to transition to Offline link state, return 0x%x\n",
				ret1);
			ret = -EINVAL;
			break;
		}
		/*
		 * The above move to physical Offline state will
		 * also move the logical state to Down.  Wait for it.
		 */
		qib_wait_linkstate(ppd, IB_PORT_DOWN, 1000);
		ret1 = set_physical_link_state(dd, WFR_PLS_DISABLED);
		if (ret1 != WFR_HCMD_SUCCESS) {
			dd_dev_err(dd,
				"Failed to transition to Disabled link state, return 0x%x\n",
				ret1);
			ret = -EINVAL;
		}
		break;
	default:
		dd_dev_info(dd, "%s: physical request 0x%x: not implemented\n",
			__func__, phys_request);
	}

	return ret;
}

static int set_ib_cfg(struct qib_pportdata *ppd, int which, u32 val)
{
	u64 reg;
	int ret = 0;

	switch (which) {
	case QIB_IB_CFG_LIDLMC:
		set_lidlmc(ppd);
		break;
	case QIB_IB_CFG_VL_HIGH_LIMIT:
		/*
		 * The VL Arbitrator high limit is sent in units of 4k
		 * bytes, while WFR stores it in units of 64 bytes.
		 */
		val *= 4096/64;
		reg = ((u64)val & WFR_SEND_HIGH_PRIORITY_LIMIT_LIMIT_MASK)
			<< WFR_SEND_HIGH_PRIORITY_LIMIT_LIMIT_SHIFT;
		write_csr(ppd->dd, WFR_SEND_HIGH_PRIORITY_LIMIT, reg);
		break;
	case QIB_IB_CFG_LSTATE:
		ret = set_link_state(ppd, val);
		break;
	case QIB_IB_CFG_LINKDEFAULT: /* IB link default (sleep/poll) */
		/* WFR only supports POLL as the default link down state */
		if (val != IB_LINKINITCMD_POLL)
			ret = -EINVAL;
		break;
	case QIB_IB_CFG_OP_VLS:
		if (ppd->vls_operational != val) {
			ppd->vls_operational = val;
			//FIXME: implement this
			//set_vls(ppd);
		}
		break;
	case QIB_IB_CFG_LWID_ENB: /* set allowed Link-width */
		ppd->link_width_enabled = val;
		//FIXME: actually set the value
		break;
        case QIB_IB_CFG_OVERRUN_THRESH: /* IB overrun threshold */
		/*
		 * WFR does not follow IB specs, save this value
		 * so we can report it, if asked.
		 */
		ppd->overrun_threshold = val;
		break;
        case QIB_IB_CFG_PHYERR_THRESH: /* IB PHY error threshold */
		/*
		 * WFR does not follow IB specs, save this value
		 * so we can report it, if asked.
		 */
		ppd->phy_error_threshold = val;
		break;

	case QIB_IB_CFG_MTU:
		set_send_length(ppd);
		break;

	case QIB_IB_CFG_PKEYS:
		if (enable_pkeys)
			set_partition_keys(ppd);
		break;

	default:
		if (print_unimplemented)
			dd_dev_info(ppd->dd,
			  "%s: which %s, val 0x%x: not implemented\n",
			  __func__, ib_cfg_name(which), val);
		break;
	}
	return ret;
}

static int set_ib_loopback(struct qib_pportdata *ppd, const char *what)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
	return 0;
}

static void get_vl_weights(struct hfi_devdata *dd, u32 target,
			   u32 size, struct ib_vl_weight_elem *vl)
{
	u64 reg;
	unsigned int i;

	for (i = 0; i < size; i++, vl++) {
		reg = read_csr(dd, target + (i * 8));

		/*
		 * NOTE: We use the low pririty shift and mask here, but
		 * they are the same for both the low and high registers.
		 */
		vl->vl = (reg >> WFR_SEND_LOW_PRIORITY_LIST_VL_SHIFT)
				& WFR_SEND_LOW_PRIORITY_LIST_VL_MASK;
		vl->weight = (reg >> WFR_SEND_LOW_PRIORITY_LIST_WEIGHT_SHIFT)
				& WFR_SEND_LOW_PRIORITY_LIST_WEIGHT_MASK;
	}
}

static void set_vl_weights(struct hfi_devdata *dd, u32 target,
			   u32 size, struct ib_vl_weight_elem *vl)
{
	u64 reg;
	unsigned int i;

	for (i = 0; i < size; i++, vl++) {
		/*
		 * NOTE: The low priority shift and mask are used here, but
		 * they are the same for both the low and high registers.
		 */
		reg = (((u64)vl->vl & WFR_SEND_LOW_PRIORITY_LIST_VL_MASK)
				<< WFR_SEND_LOW_PRIORITY_LIST_VL_SHIFT)
		      | (((u64)vl->weight
				& WFR_SEND_LOW_PRIORITY_LIST_WEIGHT_MASK)
				<< WFR_SEND_LOW_PRIORITY_LIST_WEIGHT_SHIFT);
		write_csr(dd, target + (i * 8), reg);
	}
//FIXME: Setting the weights atomatically turns this on?
	pio_send_control(dd, PSC_GLOBAL_VLARB_ENABLE);
}

/* 
 * Read one credit merge VL register.
 */
static void read_one_cm_vl(struct hfi_devdata *dd, u32 csr,
			   struct vl_limit *vll)
{
	u64 reg = read_csr(dd, csr);
	vll->dedicated = cpu_to_be16(
		(reg >> WFR_SEND_CM_CREDIT_VL_DEDICATED_LIMIT_VL_SHIFT)
		& WFR_SEND_CM_CREDIT_VL_DEDICATED_LIMIT_VL_MASK);
	vll->shared = cpu_to_be16(
		(reg >> WFR_SEND_CM_CREDIT_VL_SHARED_LIMIT_VL_SHIFT)
		& WFR_SEND_CM_CREDIT_VL_SHARED_LIMIT_VL_MASK);
}

/*
 * Read the current credit merge limits.
 */
static void get_buffer_control(
	struct hfi_devdata *dd,
	struct buffer_control *bc,
	u16 *overall_limit)
{
	u64 reg;
	int i;

	/* not all entries are filled in */
	memset(bc, 0, sizeof(*bc));

	/* STL and WFR have a 1-1 mapping */
	for (i = 0; i < WFR_TXE_NUM_DATA_VL; i++)
		read_one_cm_vl(dd, WFR_SEND_CM_CREDIT_VL + (8*i), &bc->vl[i]);

	/* NOTE: assumes that VL* and VL15 CSRs are bit-wise identical */
	read_one_cm_vl(dd, WFR_SEND_CM_CREDIT_VL15, &bc->vl[15]);

	reg = read_csr(dd, WFR_SEND_CM_GLOBAL_CREDIT);
/* TODO: defined in future HAS 0.76 */
#ifdef WFR_SEND_CM_GLOBAL_CREDIT_SHARED_LIMIT_SHIFT
	bc->overall_shared_limit = cpu_to_be16(
		(reg >> WFR_SEND_CM_GLOBAL_CREDIT_SHARED_LIMIT_SHIFT)
		& WFR_SEND_CM_GLOBAL_CREDIT_SHARED_LIMIT_MASK);
	if (overall_limit)
		*overall_limit = (reg
			>> WFR_SEND_CM_GLOBAL_CREDIT_TOTAL_CREDIT_LIMIT_SHIFT)
			& WFR_SEND_CM_GLOBAL_CREDIT_TOTAL_CREDIT_LIMIT_MASK;
#else
	{
	u16 ded_total = 0;
	u16 global_limit;

	global_limit = (reg >> WFR_SEND_CM_GLOBAL_CREDIT_GLOBAL_LIMIT_SHIFT)
			& WFR_SEND_CM_GLOBAL_CREDIT_GLOBAL_LIMIT_MASK;
	if (overall_limit)
		*overall_limit = global_limit;

	/* calculate the shared total by subtracting the summed
	   dedicated limit from the overall limit */
	for (i = 0; i < WFR_TXE_NUM_DATA_VL; i++)
		ded_total += be16_to_cpu(bc->vl[i].dedicated);
	ded_total += be16_to_cpu(bc->vl[15].dedicated);

	bc->overall_shared_limit = cpu_to_be16(global_limit - ded_total);
	}
#endif
}

#if 0
static void print_bc(struct hfi_devdata *dd, const char *func, struct buffer_control *bc, const char *what)
{
	dd_dev_info(dd, "%s: buffer control \"%s\" (hex values)\n", func, what);
	dd_dev_info(dd, "%s:   overall_shared_limit %x\n", func,
		be16_to_cpu(bc->overall_shared_limit));
	dd_dev_info(dd, "%s:   vls [%x,%x], [%x,%x], [%x,%x], [%x,%x]\n", func,
		(unsigned)be16_to_cpu(bc->vl[0].dedicated),
		(unsigned)be16_to_cpu(bc->vl[0].shared),
		(unsigned)be16_to_cpu(bc->vl[1].dedicated),
		(unsigned)be16_to_cpu(bc->vl[1].shared),
		(unsigned)be16_to_cpu(bc->vl[2].dedicated),
		(unsigned)be16_to_cpu(bc->vl[2].shared),
		(unsigned)be16_to_cpu(bc->vl[3].dedicated),
		(unsigned)be16_to_cpu(bc->vl[3].shared));
	dd_dev_info(dd, "%s:   vls [%x,%x], [%x,%x], [%x,%x], [%x,%x]\n", func,
		(unsigned)be16_to_cpu(bc->vl[4].dedicated),
		(unsigned)be16_to_cpu(bc->vl[4].shared),
		(unsigned)be16_to_cpu(bc->vl[5].dedicated),
		(unsigned)be16_to_cpu(bc->vl[5].shared),
		(unsigned)be16_to_cpu(bc->vl[6].dedicated),
		(unsigned)be16_to_cpu(bc->vl[6].shared),
		(unsigned)be16_to_cpu(bc->vl[7].dedicated),
		(unsigned)be16_to_cpu(bc->vl[7].shared));
	dd_dev_info(dd, "%s:   vls [%x,%x]\n", func,
		(unsigned)be16_to_cpu(bc->vl[15].dedicated),
		(unsigned)be16_to_cpu(bc->vl[15].shared));
}
#endif

static void nonzero_msg(struct hfi_devdata *dd, int idx, const char *what,
				u16 limit)
{
	if (limit != 0)
		dd_dev_info(dd, "Invalid %s limit %d on VL %d, ignoring\n",
			what, (int)limit, idx);
}

/* change only the shared limit portion of SendCmGLobalCredit */
static void set_global_shared(struct hfi_devdata *dd, u16 limit)
{
/* TODO: defined in future HAS 0.76 */
#ifdef WFR_SEND_CM_GLOBAL_CREDIT_SHARED_LIMIT_SHIFT
	u64 reg;

	reg = read_csr(dd, WFR_SEND_CM_GLOBAL_CREDIT);
	reg &= ~WFR_SEND_CM_GLOBAL_CREDIT_SHARED_LIMIT_SMASK;
	reg |= (u64)limit << WFR_SEND_CM_GLOBAL_CREDIT_SHARED_LIMIT_SHIFT;
	write_csr(dd, WFR_SEND_CM_GLOBAL_CREDIT, reg);
#endif
}

/* change only the total credit limit portion of SendCmGLobalCredit */
static void set_global_limit(struct hfi_devdata *dd, u16 limit)
{
	u64 reg;

	reg = read_csr(dd, WFR_SEND_CM_GLOBAL_CREDIT);
/* TODO: name changed in HAS 0.76 */
#ifdef WFR_SEND_CM_GLOBAL_CREDIT_SHARED_LIMIT_SHIFT
	reg &= ~WFR_SEND_CM_GLOBAL_CREDIT_TOTAL_CREDIT_LIMIT_SMASK;
	reg |= (u64)limit << WFR_SEND_CM_GLOBAL_CREDIT_TOTAL_CREDIT_LIMIT_SHIFT;
#else
	reg &= ~WFR_SEND_CM_GLOBAL_CREDIT_GLOBAL_LIMIT_SMASK;
	reg |= (u64)limit << WFR_SEND_CM_GLOBAL_CREDIT_GLOBAL_LIMIT_SHIFT;
#endif
	write_csr(dd, WFR_SEND_CM_GLOBAL_CREDIT, reg);
}

/* set the given per-VL shared limit */
static void set_vl_shared(struct hfi_devdata *dd, int vl, u16 limit)
{
	u64 reg;
	u32 addr;

	if (vl < WFR_TXE_NUM_DATA_VL)
		addr = WFR_SEND_CM_CREDIT_VL + (8 * vl);
	else
		addr = WFR_SEND_CM_CREDIT_VL15;

	reg = read_csr(dd, addr);
	reg &= ~WFR_SEND_CM_CREDIT_VL_SHARED_LIMIT_VL_SMASK;
	reg |= (u64)limit << WFR_SEND_CM_CREDIT_VL_SHARED_LIMIT_VL_SHIFT;
	write_csr(dd, addr, reg);
}

/* set the given per-VL dedicated limit */
static void set_vl_dedicated(struct hfi_devdata *dd, int vl, u16 limit)
{
	u64 reg;
	u32 addr;

	if (vl < WFR_TXE_NUM_DATA_VL)
		addr = WFR_SEND_CM_CREDIT_VL + (8 * vl);
	else
		addr = WFR_SEND_CM_CREDIT_VL15;

	reg = read_csr(dd, addr);
	reg &= ~WFR_SEND_CM_CREDIT_VL_DEDICATED_LIMIT_VL_SMASK;
	reg |= (u64)limit << WFR_SEND_CM_CREDIT_VL_DEDICATED_LIMIT_VL_SHIFT;
	write_csr(dd, addr, reg);
}

/* spin until the given per-VL status mask bits clear */
static void wait_for_vl_status_clear(struct hfi_devdata *dd, u64 mask)
{
	u64 reg;

	/* TODO: time out if too long? */
	while (1) {
		reg = read_csr(dd, WFR_SEND_CM_CREDIT_USED_STATUS) & mask;
		if (reg == 0)
			break;
		/* TODO: how long to delay before checking again? */
		udelay(1);
	}
}

/*
 * The number of credits on the VLs may be changed while everything
 * is "live", but the following algorithm must be followed due to
 * how the hardware is actually implemented.  In particular, 
 * Return_Credit_Status[] is the only correct status check.
 * 
 * if (reducing Global_Shared_Credit_Limit)
 *     set Global_Shared_Credit_Limit = 0
 *     use_all_vl = 1
 * mask0 = all VLs that are changing either dedicated or shared limits
 * set Shared_Limit[mask0] = 0
 * spin until Return_Credit_Status[use_all_vl ? all VL : mask0] == 0
 * if (changing any dedicated limit)
 *     mask1 = all VLs that are lowering dedicated limits
 *     lower Dedicated_Limit[mask1]
 *     spin until Return_Credit_Status[mask1] == 0
 *     raise Dedicated_Limits
 * raise Shared_Limits
 * raise Global_Shared_Credit_Limit
 * 
 * lower = if the new limit is lower, set the limit to the new value
 * raise = if the new limit is higher than the current value (may be changed
 * 	earlier in the algorithm), set the new limit to the new value
 */
static int set_buffer_control(struct hfi_devdata *dd,
			       struct buffer_control *new_bc)
{
	u64 changing_mask, ld_mask, stat_mask;
	int change_count;
	int i, use_all_mask;
	struct buffer_control cur_bc;
	u8 changing[STL_MAX_VLS];
	u8 lowering_dedicated[STL_MAX_VLS];
	u16 cur_total;
	u32 new_total = 0;
	const u64 all_mask =
		WFR_SEND_CM_CREDIT_USED_STATUS_VL0_RETURN_CREDIT_STATUS_SMASK
		| WFR_SEND_CM_CREDIT_USED_STATUS_VL1_RETURN_CREDIT_STATUS_SMASK
		| WFR_SEND_CM_CREDIT_USED_STATUS_VL2_RETURN_CREDIT_STATUS_SMASK
		| WFR_SEND_CM_CREDIT_USED_STATUS_VL3_RETURN_CREDIT_STATUS_SMASK
		| WFR_SEND_CM_CREDIT_USED_STATUS_VL4_RETURN_CREDIT_STATUS_SMASK
		| WFR_SEND_CM_CREDIT_USED_STATUS_VL5_RETURN_CREDIT_STATUS_SMASK
		| WFR_SEND_CM_CREDIT_USED_STATUS_VL6_RETURN_CREDIT_STATUS_SMASK
		| WFR_SEND_CM_CREDIT_USED_STATUS_VL7_RETURN_CREDIT_STATUS_SMASK
		| WFR_SEND_CM_CREDIT_USED_STATUS_VL15_RETURN_CREDIT_STATUS_SMASK;

#define valid_vl(idx) ((idx) < WFR_TXE_NUM_DATA_VL || (idx) == 15)
#define NUM_USABLE_VLS 16	/* look at VL15 and less */


	/* find the new total credits, do sanity check on unused VLs */
	for (i = 0; i < STL_MAX_VLS; i++) {
		if (valid_vl(i)) {
			new_total += be16_to_cpu(new_bc->vl[i].dedicated);
			continue;
		}
		nonzero_msg(dd, i, "dedicated",
			be16_to_cpu(new_bc->vl[i].dedicated));
		nonzero_msg(dd, i, "shared",
			be16_to_cpu(new_bc->vl[i].shared));
		new_bc->vl[i].dedicated = 0;
		new_bc->vl[i].shared = 0;
	}
	new_total += be16_to_cpu(new_bc->overall_shared_limit);
	if (new_total > (u32)dd->link_credits)
		return -EINVAL;
	/* fetch the curent values */
	get_buffer_control(dd, &cur_bc, &cur_total);

	/*
	 * Create the masks we will use.
	 */
	memset(changing, 0, sizeof(changing));
	memset(lowering_dedicated, 0, sizeof(changing));
	/* NOTE: Assumes that the individual VL bits are adjacent and in
	   increasing order */
	stat_mask = WFR_SEND_CM_CREDIT_USED_STATUS_VL0_RETURN_CREDIT_STATUS_SMASK;
	changing_mask = 0;
	ld_mask = 0;
	change_count = 0;
	for (i = 0; i < NUM_USABLE_VLS; i++, stat_mask <<= 1) {
		if (!valid_vl(i))
			continue;
		if (
/* TODO: real code for HAS 0.76, workaround for before */
#ifdef WFR_SEND_CM_GLOBAL_CREDIT_SHARED_LIMIT_SHIFT
		    new_bc->vl[i].dedicated
					!= cur_bc.vl[i].dedicated
				|| new_bc->vl[i].shared
					!= cur_bc.vl[i].shared
#else
/*
 * Hack: if the sim does not have a shared limit, we can't easily reduce it
 * to zero to force "using shared credits bits" to go to zero, so do the
 * next best thing: say all the VLs are changing, which will zero the
 * shared credit limit on all VLs, removing all use of shared space
 * and allowing the wait so succeed.
 */  
		    1
#endif
					) {
			changing[i] = 1;
			changing_mask |= stat_mask;
			change_count++;
		}
		if (be16_to_cpu(new_bc->vl[i].dedicated) <
					be16_to_cpu(cur_bc.vl[i].dedicated)) {
			lowering_dedicated[i] = 1;
			ld_mask |= stat_mask;
		}
	}

	/* bracket the credit change with a total adjustment */
	if (new_total > cur_total)
		set_global_limit(dd, new_total);

	/*
	 * Start the credit change algorithm.
	 */
	use_all_mask = 0;
	if (be16_to_cpu(new_bc->overall_shared_limit) <
			be16_to_cpu(cur_bc.overall_shared_limit)) {
		set_global_shared(dd, 0);
		cur_bc.overall_shared_limit = 0;
		use_all_mask = 1;
	}

	for (i = 0; i < NUM_USABLE_VLS; i++) {
		if (!valid_vl(i))
			continue;

		if (changing[i]) {
			set_vl_shared(dd, i, 0);
			cur_bc.vl[i].shared = 0;
		}
	}

	wait_for_vl_status_clear(dd, use_all_mask ? all_mask : changing_mask);

	if (change_count > 0) {
		for (i = 0; i < NUM_USABLE_VLS; i++) {
			if (!valid_vl(i))
				continue;

			if (lowering_dedicated[i]) {
				set_vl_dedicated(dd, i, 
					be16_to_cpu(new_bc->vl[i].dedicated));
				cur_bc.vl[i].dedicated =
						new_bc->vl[i].dedicated;
			}
		}

		wait_for_vl_status_clear(dd, ld_mask);

		/* now raise all dedicated that are going up */
		for (i = 0; i < NUM_USABLE_VLS; i++) {
			if (!valid_vl(i))
				continue;

			if (be16_to_cpu(new_bc->vl[i].dedicated) >
					be16_to_cpu(cur_bc.vl[i].dedicated))
				set_vl_dedicated(dd, i, 
					be16_to_cpu(new_bc->vl[i].dedicated));
		}
	}

	/* next raise all shared that are going up */
	for (i = 0; i < NUM_USABLE_VLS; i++) {
		if (!valid_vl(i))
			continue;

		if (be16_to_cpu(new_bc->vl[i].shared) >
				be16_to_cpu(cur_bc.vl[i].shared))
			set_vl_shared(dd, i, be16_to_cpu(new_bc->vl[i].shared));
	}

	/* finally raise the global shared */
	if (be16_to_cpu(new_bc->overall_shared_limit) >
			be16_to_cpu(cur_bc.overall_shared_limit))
		set_global_shared(dd,
			be16_to_cpu(new_bc->overall_shared_limit));

	/* bracket the credit change with a total adjustment */
	if (new_total < cur_total)
		set_global_limit(dd, new_total);
	return 0;
}

/*
 * Read the given fabric manager table.
 */
int fm_get_table(struct qib_pportdata *ppd, int which, void *t)
{
	switch (which) {
	case FM_TBL_VL_HIGH_ARB:
		get_vl_weights(ppd->dd, WFR_SEND_HIGH_PRIORITY_LIST,
			WFR_VL_ARB_HIGH_PRIO_TABLE_SIZE, t);
		break;
	case FM_TBL_VL_LOW_ARB:
		get_vl_weights(ppd->dd, WFR_SEND_LOW_PRIORITY_LIST,
			WFR_VL_ARB_LOW_PRIO_TABLE_SIZE, t);
		break;
	case FM_TBL_BUFFER_CONTROL:
		get_buffer_control(ppd->dd, t, NULL);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/*
 * Write the given fabric manager table.
 */
int fm_set_table(struct qib_pportdata *ppd, int which, void *t)
{
	int ret = 0;

	switch (which) {
	case FM_TBL_VL_HIGH_ARB:
		set_vl_weights(ppd->dd, WFR_SEND_HIGH_PRIORITY_LIST,
			WFR_VL_ARB_HIGH_PRIO_TABLE_SIZE, t);
		break;
	case FM_TBL_VL_LOW_ARB:
		set_vl_weights(ppd->dd, WFR_SEND_LOW_PRIORITY_LIST,
			WFR_VL_ARB_LOW_PRIO_TABLE_SIZE, t);
		break;
	case FM_TBL_BUFFER_CONTROL:
		ret = set_buffer_control(ppd->dd, t);
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static void update_usrhead(struct qib_ctxtdata *rcd, u64 hd,
				    u32 updegr, u32 egrhd, u32 npkts)
{
	struct hfi_devdata *dd = rcd->dd;
	u64 reg;
	u32 ctxt = rcd->ctxt;

//FIXME: no timeout adjustment code yet
//	+ not that great that we mix in the update flag with hd
#if 0
	/*
	 * Need to write timeout register before updating rcvhdrhead to ensure
	 * that the timer is enabled on reception of a packet.
	 */
	if (hd >> IBA7322_HDRHEAD_PKTINT_SHIFT)
		adjust_rcv_timeout(rcd, npkts);
#endif
	if (updegr) {
		reg = (egrhd & WFR_RCV_EGR_INDEX_HEAD_HEAD_MASK)
			<< WFR_RCV_EGR_INDEX_HEAD_HEAD_SHIFT;
		write_uctxt_csr(dd, ctxt, WFR_RCV_EGR_INDEX_HEAD, reg);
	}
	mmiowb();
	reg = ((u64)rcv_intr_count << WFR_RCV_HDR_HEAD_COUNTER_SHIFT) |
		((hd & WFR_RCV_HDR_HEAD_HEAD_MASK)
			<< WFR_RCV_HDR_HEAD_HEAD_SHIFT);
	write_uctxt_csr(dd, ctxt, WFR_RCV_HDR_HEAD, reg);
	mmiowb();

}

static u32 hdrqempty(struct qib_ctxtdata *rcd)
{
	if (print_unimplemented)
		dd_dev_info(rcd->dd, "%s: not implemented\n", __func__);
	return 0; /* not empty */
}

/*
 * Context Control and Receive Array encoding for buffer size:
 *	0x0 invalid
 *	0x1   4 KB
 *	0x2   8 KB
 *	0x3  16 KB
 *	0x4  32 KB
 *	0x5  64 KB
 *	0x6 128 KB
 *	0x7 256 KB
 *	0x8 512 KB (Receive Array only)
 *	0x9   1 MB (Receive Array only)
 *	0xa   2 MB (Receive Array only)
 *
 *	0xB-0xF - reserved (Receive Array only)
 *	
 *
 * This routine assumes that the value has already been sanity checked.
 */
static u32 encoded_size(u32 size)
{
#if 1
	switch (size) {
	case   4*1024: return 0x1;
	case   8*1024: return 0x2;
	case  16*1024: return 0x3;
	case  32*1024: return 0x4;
	case  64*1024: return 0x5;
	case 128*1024: return 0x6;
	case 256*1024: return 0x7;
	case 512*1024: return 0x8;
	case   1*1024*1024: return 0x9;
	case   2*1024*1024: return 0xa;
	}
	return 0x1;	/* if invalid, go with the minimum size */
#else
	/* this should be the same, but untested and arguably slower */
	if (size < 4*1024)
		return 0x1;	/* shouldn't happen, but.. */
	return ilog2(size) - 11;
#endif
}

/* TODO: This should go away once everythig is implemented */
static void rcvctrl_unimplemented(struct hfi_devdata *dd, const char *func, int ctxt, unsigned int op, int *header_printed, const char *what)
{
	if (*header_printed == 0) {
		*header_printed = 1;
		dd_dev_info(dd, "%s: context %d, op 0x%x\n", func, ctxt, op);
	}
	dd_dev_info(dd, "    %s ** NOT IMPLEMENTED **\n", what);
}

/*
 * TODO: What about these fields in WFR_RCV_CTXT_CTRL?
 *	ThHdrQueueWrites
 *	ThEagerPayloadWrites
 *	ThTIDPayloadWrites
 *	ThRcvHdrTailWrite
 *	Redirect
 *	DontDropRHQFull
 *	DontDropEgrFull
 */
static void rcvctrl(struct hfi_devdata *dd, unsigned int op, int ctxt)
{
	struct qib_ctxtdata *rcd;
	u64 rcvctrl, reg;
	int hp = 0;			/* header printed? */
	int did_enable = 0;

	rcd = dd->rcd[ctxt];
	if (!rcd)
		return;

	dd_dev_info(dd, "ctxt %d op 0x%x", ctxt, op);
	// FIXME: QIB_RCVCTRL_BP_ENB/DIS
	if (op & QIB_RCVCTRL_BP_ENB)
		rcvctrl_unimplemented(dd, __func__, ctxt, op, &hp, "RCVCTRL_BP_ENB");
	if (op & QIB_RCVCTRL_BP_DIS)
		rcvctrl_unimplemented(dd, __func__, ctxt, op, &hp, "RCVCTRL_BP_DIS");

	/* XXX (Mitko): Do we want to use the shadow value here? */
	rcvctrl = read_kctxt_csr(dd, ctxt, WFR_RCV_CTXT_CTRL);
	/* if the context already enabled, don't do the extra steps */
	if ((op & QIB_RCVCTRL_CTXT_ENB)
			&& !(rcvctrl & WFR_RCV_CTXT_CTRL_ENABLE_SMASK)) {
		/* reset the tail and hdr addresses, and sequence count */
		write_kctxt_csr(dd, ctxt, WFR_RCV_HDR_ADDR,
				rcd->rcvhdrq_phys);
		if (!(dd->flags & QIB_NODMA_RTAIL)) {
			write_kctxt_csr(dd, ctxt, WFR_RCV_HDR_TAIL_ADDR,
					rcd->rcvhdrqtailaddr_phys);
		}
		rcd->seq_cnt = 1;

		/* enable the context */
		rcvctrl |= WFR_RCV_CTXT_CTRL_ENABLE_SMASK;

		/* set the control's eager buffer size */
		rcvctrl &= ~WFR_RCV_CTXT_CTRL_EGR_BUF_SIZE_SMASK;
		rcvctrl |= (encoded_size(rcd->rcvegrbuf_size)
				& WFR_RCV_CTXT_CTRL_EGR_BUF_SIZE_MASK)
					<< WFR_RCV_CTXT_CTRL_EGR_BUF_SIZE_SHIFT;

		/* zero interrupt timeout - set after enable */
		write_kctxt_csr(dd, ctxt, WFR_RCV_AVAIL_TIME_OUT, 0);

		/* zero RcvHdrHead - set RcvHdrHead.Counter after enable */
		write_uctxt_csr(dd, ctxt, WFR_RCV_HDR_HEAD, 0);
		did_enable = 1;

		/* zero RcvEgrIndexHead */
		write_uctxt_csr(dd, ctxt, WFR_RCV_EGR_INDEX_HEAD, 0);

		/* set eager count and base index */
		reg = (((rcd->eager_count >> WFR_RCV_SHIFT)
					& WFR_RCV_EGR_CTRL_EGR_CNT_MASK)
				<< WFR_RCV_EGR_CTRL_EGR_CNT_SHIFT) |
		      (((rcd->eager_base >> WFR_RCV_SHIFT)
					& WFR_RCV_EGR_CTRL_EGR_BASE_INDEX_MASK)
				<< WFR_RCV_EGR_CTRL_EGR_BASE_INDEX_SHIFT);
		write_kctxt_csr(dd, ctxt, WFR_RCV_EGR_CTRL, reg);

		/*
		 * Set TID (expected) count and base index.
		 * rcd->expected_count is set to individual RcvArray entries,
		 * not pairs, and the CSR takes a pair-count in groups of
		 * four, so divide by 8.
		 */
		reg = (((rcd->expected_count >> WFR_RCV_SHIFT)
					& WFR_RCV_TID_CTRL_TID_PAIR_CNT_MASK)
				<< WFR_RCV_TID_CTRL_TID_PAIR_CNT_SHIFT) |
		      (((rcd->expected_base >> WFR_RCV_SHIFT)
					& WFR_RCV_TID_CTRL_TID_BASE_INDEX_MASK)
				<< WFR_RCV_TID_CTRL_TID_BASE_INDEX_SHIFT);
		write_kctxt_csr(dd, ctxt, WFR_RCV_TID_CTRL, reg);
	}
	if (op & QIB_RCVCTRL_CTXT_DIS)
		rcvctrl &= ~WFR_RCV_CTXT_CTRL_ENABLE_SMASK;
	if (op & QIB_RCVCTRL_INTRAVAIL_ENB)
		rcvctrl |= WFR_RCV_CTXT_CTRL_INTR_AVAIL_SMASK;
	if (op & QIB_RCVCTRL_INTRAVAIL_DIS)
		rcvctrl &= ~WFR_RCV_CTXT_CTRL_INTR_AVAIL_SMASK;
	if (op & QIB_RCVCTRL_TAILUPD_ENB)
		rcvctrl |= WFR_RCV_CTXT_CTRL_TAIL_UPD_SMASK;
	if (op & QIB_RCVCTRL_TAILUPD_DIS)
		rcvctrl &= ~WFR_RCV_CTXT_CTRL_TAIL_UPD_SMASK;
	if (op & QIB_RCVCTRL_TIDFLOW_ENB)
		rcvctrl |= WFR_RCV_CTXT_CTRL_TID_FLOW_ENABLE_SMASK;
	if (op & QIB_RCVCTRL_TIDFLOW_DIS)
		rcvctrl &= ~WFR_RCV_CTXT_CTRL_TID_FLOW_ENABLE_SMASK;
	if (op & QIB_RCVCTRL_ONE_PKT_EGR_ENB)
		rcvctrl |= WFR_RCV_CTXT_CTRL_ONE_PACKET_PER_EGR_BUFFER_SMASK;
	if (op & QIB_RCVCTRL_ONE_PKT_EGR_DIS)
		rcvctrl &= ~WFR_RCV_CTXT_CTRL_ONE_PACKET_PER_EGR_BUFFER_SMASK;
	if (op & QIB_RCVCTRL_NO_RHQ_DROP_ENB)
		rcvctrl |= WFR_RCV_CTXT_CTRL_DONT_DROP_RHQ_FULL_SMASK;
	if (op & QIB_RCVCTRL_NO_RHQ_DROP_DIS)
		rcvctrl &= ~WFR_RCV_CTXT_CTRL_DONT_DROP_RHQ_FULL_SMASK;
	if (op & QIB_RCVCTRL_NO_EGR_DROP_ENB)
		rcvctrl |= WFR_RCV_CTXT_CTRL_DONT_DROP_EGR_FULL_SMASK;
	if (op & QIB_RCVCTRL_NO_EGR_DROP_DIS)
		rcvctrl &= ~WFR_RCV_CTXT_CTRL_DONT_DROP_EGR_FULL_SMASK;
	rcd->rcvctrl = rcvctrl;
	dd_dev_info(dd, "ctxt %d rcvctrl 0x%llx\n", ctxt, rcvctrl);
	write_kctxt_csr(dd, ctxt, WFR_RCV_CTXT_CTRL, rcd->rcvctrl);

	if (did_enable) {
		/*
		 * The interrupt timeout and count must be set after
		 * the context is enabled to take effect.
		 */
		/* set interrupt timeout */
		write_kctxt_csr(dd, ctxt, WFR_RCV_AVAIL_TIME_OUT,
			(u64)rcv_intr_timeout <<
				WFR_RCV_AVAIL_TIME_OUT_TIME_OUT_RELOAD_SHIFT);

		/* set RcvHdrHead.Counter, zero RcvHdrHead.Head (again) */
		reg = (u64)rcv_intr_count << WFR_RCV_HDR_HEAD_COUNTER_SHIFT;
		write_uctxt_csr(dd, ctxt, WFR_RCV_HDR_HEAD, reg);
	}
}

static u64 portcntr(struct qib_pportdata *ppd, u32 reg)
{
	static int called;
	if (!called) {
		called = 1;
		if (print_unimplemented)
			dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
	}
	return 0;
}

static u32 read_cntrs(struct hfi_devdata *dd, loff_t pos, char **namep,
			      u64 **cntrp)
{
	int ret;

	if (namep) {
		ret = dd->cntrnameslen;
		if (pos >= ret)
			return 0;
		*namep = dd->cntrnames;
	} else {
		const struct cntr_entry *entry;
		u64 *cntr;
		int i;

		ret = ARRAY_SIZE(dev_cntrs) * sizeof(u64);
		if (pos >= ret)
			return 0;

		cntr = *cntrp = dd->cntrs;
		for (entry = &dev_cntrs[0], i = 0;
		     i < ARRAY_SIZE(dev_cntrs); i++, entry++)
			*cntr++ = entry->read_cntr(entry, dd);
	}
	return ret;
}

static u32 read_portcntrs(struct hfi_devdata *dd, loff_t pos, u32 port,
				  char **namep, u64 **cntrp)
{
	int ret;

	if (namep) {
		ret = dd->portcntrnameslen;
		if (pos >= ret)
			return 0;
		*namep = dd->portcntrnames;
	} else {
		const struct cntr_entry *entry;
		struct qib_pportdata *ppd;
		u64 *cntr;
		int i;

		ret = (dd->nportcntrs) * sizeof(u64);
		if (pos >= ret)
			return 0;
		ppd = (struct qib_pportdata *)(dd + 1 + port);
		cntr = *cntrp = ppd->cntrs;
		for (entry = &port_cntrs[0], i = 0;
		     i < dd->nportcntrs; i++, entry++)
			*cntr++ = entry->read_cntr(entry, ppd);
	}
	return ret; /* final read after getting everything */
}

static void free_cntrs(struct hfi_devdata *dd)
{
	struct qib_pportdata *ppd;
	int i;

	if (dd->stats_timer.data)
		del_timer_sync(&dd->stats_timer);
	dd->stats_timer.data = 0;
	ppd = (struct qib_pportdata *)(dd + 1);
	for (i = 0; i < dd->num_pports; i++, ppd++) {
		kfree(ppd->cntrs);
		ppd->cntrs = NULL;
	}
	kfree(dd->portcntrnames);
	dd->portcntrnames = NULL;
	kfree(dd->cntrs);
	dd->cntrs = NULL;
	kfree(dd->cntrnames);
	dd->cntrnames = NULL;
}

static void get_faststats(unsigned long opaque)
{
	static int called;
	struct hfi_devdata *dd = (struct hfi_devdata *) opaque;
	if (!called) {
		called = 1;
		if (print_unimplemented)
			dd_dev_info(dd, "%s: not implemented\n", __func__);
	}
	mod_timer(&dd->stats_timer, jiffies + HZ * ACTIVITY_TIMER);
}

static int init_cntrs(struct hfi_devdata *dd)
{
	int i;
	size_t sz;
	char *p;
	struct qib_pportdata *ppd;

	/* set up the stats timer; the add_timer is done at end of init */
	init_timer(&dd->stats_timer);
	dd->stats_timer.function = get_faststats;
	dd->stats_timer.data = (unsigned long) dd;

	/* per device */
	dd->cntrs = kzalloc(sizeof(u64) * ARRAY_SIZE(dev_cntrs), GFP_KERNEL);
	if (!dd->cntrs)
		goto bail;
	/* size cntrnames */
	for (sz = 0, i = 0; i < ARRAY_SIZE(dev_cntrs); i++)
		/* +1 for newline  */
		sz += strlen(dev_cntrs[i].name) + 1;
	dd->cntrnameslen = sz;
	dd->cntrnames = kmalloc(sz, GFP_KERNEL);
	if (!dd->cntrnames)
		goto bail;
	/* fill in cntrnames */
	for (p = dd->cntrnames, i = 0; i < ARRAY_SIZE(dev_cntrs); i++) {
		memcpy(p, dev_cntrs[i].name, strlen(dev_cntrs[i].name));
		p += strlen(dev_cntrs[i].name);
		*p++ = '\n';
	}
	/* size port counter names */
	/* assume context overflows are last and adjust */
	dd->nportcntrs = ARRAY_SIZE(port_cntrs) + dd->num_rcv_contexts - 160;
	for (sz = 0, i = 0; i < dd->nportcntrs; i++)
		/* +1 for newline  */
		sz += strlen(port_cntrs[i].name) + 1;
	dd->portcntrnameslen = sz;
	dd->portcntrnames = kmalloc(sz, GFP_KERNEL);
	if (!dd->portcntrnames)
		goto bail;
	for (p = dd->portcntrnames, i = 0; i < dd->nportcntrs; i++) {
		memcpy(p, port_cntrs[i].name, strlen(port_cntrs[i].name));
		p += strlen(port_cntrs[i].name);
		*p++ = '\n';
	}
	/* per port */
	ppd = (struct qib_pportdata *)(dd + 1);
	for (i = 0; i < dd->num_pports; i++, ppd++) {
		ppd->cntrs = kzalloc(sizeof(u64) * dd->nportcntrs,
				     GFP_KERNEL);
		if (!ppd->cntrs)
			goto bail;
	}
	mod_timer(&dd->stats_timer, jiffies + HZ * ACTIVITY_TIMER);
	return 0;
bail:
	free_cntrs(dd);
	return -ENOMEM;
}

static void xgxs_reset(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
}

static u32 chip_to_ib_lstate(struct hfi_devdata *dd, u32 chip_lstate)
{
	switch (chip_lstate) {
	default:
		dd_dev_err(dd, "Unknown chip logical state 0x%x, reporting IB_PORT_DOWN\n", chip_lstate);
		/* fall through */
	case WFR_LSTATE_DOWN:
		return IB_PORT_DOWN;
	case WFR_LSTATE_INIT:
		return IB_PORT_INIT;
	case WFR_LSTATE_ARMED:
		return IB_PORT_ARMED;
	case WFR_LSTATE_ACTIVE:
		return IB_PORT_ACTIVE;
	}
}

static u32 chip_to_ib_pstate(struct hfi_devdata *dd, u32 chip_pstate)
{
	/* look at the WFR meta-states only */
	switch (chip_pstate & 0xf0) {
	default:
		dd_dev_err(dd, "Unexpected chip physical state of 0x%x\n",
			chip_pstate);
		/* fall through */
	case WFR_PLS_DISABLED:
		return IB_PORTPHYSSTATE_DISABLED;
	case WFR_PLS_OFFLINE:
		/*
		 * There is no IB equivalent for Offline.  The closest
		 * is Disabled, so use that.
		 */
		return IB_PORTPHYSSTATE_DISABLED;
	case WFR_PLS_POLLING:
		return IB_PORTPHYSSTATE_POLL;
	case WFR_PLS_CONFIGPHY:
		return IB_PORTPHYSSTATE_CFG_TRAIN;
	case WFR_PLS_LINKUP:
		return IB_PORTPHYSSTATE_LINKUP;
	case WFR_PLS_PHYTEST:
		return IB_PORTPHYSSTATE_PHY_TEST;
	}
}

/* return the IB port logical state name */
static const char *ib_lstate_name(u32 lstate)
{
	static const char *ib_port_logical_names[] = {
		"IB_PORT_NOP",
		"IB_PORT_DOWN",
		"IB_PORT_INIT",
		"IB_PORT_ARMED",
		"IB_PORT_ACTIVE",
		"IB_PORT_ACTIVE_DEFER",
	};
	if (lstate < ARRAY_SIZE(ib_port_logical_names))
		return ib_port_logical_names[lstate];
	return "unknown";
}

/* return the IB port physical state name */
static const char *ib_pstate_name(u32 pstate)
{
	static const char *ib_port_physical_names[] = {
		"IB_PHYS_NOP",
		"IB_PHYS_SLEEP",
		"IB_PHYS_POLL",
		"IB_PHYS_DISABLED",
		"IB_PHYS_CFG_TRAIN",
		"IB_PHYS_LINKUP",
		"IB_PHYS_LINK_ERR_RECOVER",
		"IB_PHYS_PHY_TEST",
	};
	if (pstate < ARRAY_SIZE(ib_port_physical_names))
		return ib_port_physical_names[pstate];
	return "unknown";
}

/* read and return the logical IB port state */
static u32 iblink_state(struct qib_pportdata *ppd)
{
	u32 new_state;

	new_state = chip_to_ib_lstate(ppd->dd, read_logical_state(ppd->dd));
	if (new_state != ppd->lstate) {
		dd_dev_info(ppd->dd, "%s: logical state changed to %s (0x%x)\n",
			__func__, ib_lstate_name(new_state), new_state);
		ppd->lstate = new_state;
	}
	/*
	 * Set port status flags in the page mapped into userspace
	 * memory. Do it here to ensure a reliable state - this is
	 * the only function called by all state handling code.
	 * Always set the flags due to the fact that the cache value
	 * might have been changed explicitly outside of this
	 * function.
	 */
	if (ppd->statusp) {
		switch(ppd->lstate) {
		case IB_PORT_DOWN:
		case IB_PORT_INIT:
			*ppd->statusp &= ~(QIB_STATUS_IB_CONF |
					   QIB_STATUS_IB_READY);
			break;
		case IB_PORT_ARMED:
			*ppd->statusp |= QIB_STATUS_IB_CONF;
			break;
		case IB_PORT_ACTIVE:
			*ppd->statusp |= QIB_STATUS_IB_READY;
			break;
		}
	}
	return ppd->lstate;
}

static u8 ibphys_portstate(struct qib_pportdata *ppd)
{
	static u32 remembered_state = 0xff;
	u32 pstate;
	u32 ib_pstate;

	pstate = read_physical_state(ppd->dd);
	ib_pstate = chip_to_ib_pstate(ppd->dd, pstate);
	if (remembered_state != ib_pstate) {
		dd_dev_info(ppd->dd,
			"%s: physical state changed to %s (0x%x), phy 0x%x\n",
			__func__, ib_pstate_name(ib_pstate), ib_pstate, pstate);
		remembered_state = ib_pstate;
	}
	return ib_pstate;
}

static int gpio_mod(struct hfi_devdata *dd, u32 out, u32 dir, u32 mask)
{
	if (print_unimplemented)
		dd_dev_info(dd, "%s: not implemented\n", __func__);
	/* return non-zero to indicate positive progress */
	return 1;
}

static void set_cntr_sample(struct qib_pportdata *ppd, u32 intv,
				     u32 start)
{
	static int called;
	if (!called) {
		called = 1;
		if (print_unimplemented)
			dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
	}
}

static void sdma_set_desc_cnt(struct qib_pportdata *ppd, unsigned cnt)
{
	u64 reg = cnt;

#ifdef JAG_SDMA_VERBOSITY
dd_dev_err(ppd->dd, "JAG SDMA %s:%d %s()\n", __FILE__, __LINE__, __func__);
#endif

	reg &= WFR_SEND_DMA_DESC_CNT_CNT_MASK;
	reg <<= WFR_SEND_DMA_RELOAD_CNT_CNT_SHIFT;
	write_kctxt_csr(ppd->dd, 0, WFR_SEND_DMA_DESC_CNT, reg);
}

static const struct sdma_set_state_action sdma_action_table[] = {
	[qib_sdma_state_s00_hw_down] = {
		.go_s99_running_tofalse = 1,
		.op_enable = 0,
		.op_intenable = 0,
		.op_halt = 0,
		.op_cleanup = 0,
		.op_drain = 0,
	},
	[qib_sdma_state_s10_hw_start_up_halt_wait] = {
		.op_enable = 0,
		.op_intenable = 1,
		.op_halt = 1,
		.op_cleanup = 0,
		.op_drain = 0,
	},
	[qib_sdma_state_s15_hw_start_up_clean_wait] = {
		.op_enable = 0,
		.op_intenable = 1,
		.op_halt = 0,
		.op_cleanup = 1,
		.op_drain = 0,
	},
	[qib_sdma_state_s20_idle] = {
		.op_enable = 0,
		.op_intenable = 1,
		.op_halt = 0,
		.op_cleanup = 0,
		.op_drain = 0,
	},
	[qib_sdma_state_s30_sw_clean_up_wait] = {
		.op_enable = 0,
		.op_intenable = 1,
		.op_halt = 0,
		.op_cleanup = 0,
		.op_drain = 0,
	},
	[qib_sdma_state_s40_hw_clean_up_wait] = {
		.op_enable = 1,
		.op_intenable = 1,
		.op_halt = 0,
		.op_cleanup = 0,
		.op_drain = 0,
	},
	[qib_sdma_state_s50_hw_halt_wait] = {
		.op_enable = 1,
		.op_intenable = 1,
		.op_halt = 1,
		.op_cleanup = 0,
		.op_drain = 1,
	},
	[qib_sdma_state_s99_running] = {
		.op_enable = 1,
		.op_intenable = 1,
		.op_halt = 0,
		.op_cleanup = 0,
		.op_drain = 0,
		.go_s99_running_totrue = 1,
	},
};

static void sdma_init_early(struct qib_pportdata *ppd)
{
#ifdef JAG_SDMA_VERBOSITY
dd_dev_err(ppd->dd, "JAG SDMA %s:%d %s()\n", __FILE__, __LINE__, __func__);
#endif

	ppd->sdma_state.set_state_action = sdma_action_table;
}

static void init_sdma_regs(struct qib_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;

#ifdef JAG_SDMA_VERBOSITY
dd_dev_err(ppd->dd, "JAG SDMA %s:%d %s()\n", __FILE__, __LINE__, __func__);
#endif

	write_kctxt_csr(dd, 0, WFR_SEND_DMA_BASE_ADDR, ppd->sdma_descq_phys);
	sdma_setlengen(ppd);
	sdma_update_tail(ppd, 0); /* Set SendDmaTail */
	write_kctxt_csr(dd, 0, WFR_SEND_DMA_RELOAD_CNT, sdma_idle_cnt);
	write_kctxt_csr(dd, 0, WFR_SEND_DMA_DESC_CNT, 0);
	write_kctxt_csr(dd, 0, WFR_SEND_DMA_HEAD_ADDR, ppd->sdma_head_phys);
	write_kctxt_csr(dd, 0, WFR_SEND_DMA_MEMORY, DEFAULT_WFR_SEND_DMA_MEMORY);
	write_kctxt_csr(dd, 0, WFR_SEND_DMA_ENG_ERR_MASK, ~0ull);
	write_kctxt_csr(dd, 0, WFR_SEND_DMA_CHECK_ENABLE,
			HFI_PKT_BASE_SDMA_INTEGRITY);
}

static u16 sdma_gethead(struct qib_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;
	int sane;
	int use_dmahead;
	u16 swhead;
	u16 swtail;
	u16 cnt;
	u16 hwhead;

#ifdef JAG_SDMA_VERBOSITY
dd_dev_err(ppd->dd, "JAG SDMA %s:%d %s()\n", __FILE__, __LINE__, __func__);
#endif

	use_dmahead = __qib_sdma_running(ppd) &&
		(dd->flags & QIB_HAS_SDMA_TIMEOUT);
retry:
	hwhead = use_dmahead ?
		(u16) le64_to_cpu(*ppd->sdma_head_dma) :
		(u16) read_kctxt_csr(dd, 0, WFR_SEND_DMA_HEAD);

	swhead = ppd->sdma_descq_head;
	swtail = ppd->sdma_descq_tail;
	cnt = ppd->sdma_descq_cnt;

	if (swhead < swtail)
		/* not wrapped */
		sane = (hwhead >= swhead) & (hwhead <= swtail);
	else if (swhead > swtail)
		/* wrapped around */
		sane = ((hwhead >= swhead) && (hwhead < cnt)) ||
			(hwhead <= swtail);
	else
		/* empty */
		sane = (hwhead == swhead);

	if (unlikely(!sane)) {
		dd_dev_err(dd, "IB%u:%u bad head %s hwhd=%hu swhd=%hu swtl=%hu "
			"cnt=%hu\n", dd->unit, ppd->port,
			use_dmahead ? "(dma)" : "(kreg)",
			hwhead, swhead, swtail, cnt);
		if (use_dmahead) {
			/* try one more time, directly from the register */
			use_dmahead = 0;
			goto retry;
		}
		/* proceed as if no progress */
		hwhead = swhead;
	}

	return hwhead;
}

static int sdma_busy(struct qib_pportdata *ppd)
{

#ifdef JAG_SDMA_VERBOSITY
dd_dev_err(ppd->dd, "JAG SDMA %s:%d %s()\n", __FILE__, __LINE__, __func__);
#endif

	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
	return 0;
}

/*
 * QIB sets these rcd fields in this function:
 *	rcvegrcnt	 (now eager_count)
 *	rcvegr_tid_base  (now eager_base)
 */
static int init_ctxt(struct qib_ctxtdata *rcd)
{
	struct hfi_devdata *dd = rcd->dd;
	u32 context = rcd->ctxt, max_entries, eager_base;
	u16 ngroups = rcd->rcv_array_groups;
	int ret = 0;

	/*
	 * Simple allocation: we have already pre-allocated the number
	 * of RcvArray entry groups. Each ctxtdata structure holds the
	 * number of groups for that context.
	 *
	 * To follow CSR requirements and maintain cacheline alignment,
	 * make sure all sizes and bases are multiples of group_size.
	 *
	 * The expected entry count is what is left after assigning eager.
	 */
	eager_base = context * ngroups * dd->rcv_entries.group_size;
	if (context >= dd->first_user_ctxt &&
	    (context - dd->first_user_ctxt) <
	    dd->rcv_entries.nctxt_extra) {
		eager_base += (context - dd->first_user_ctxt) *
			dd->rcv_entries.group_size;
	}
	dd_dev_info(dd, "ctxt%u: total ctxt ngroups %u\n",
		    context, ngroups);

	max_entries = ngroups * dd->rcv_entries.group_size;

	if (rcd->eager_count > WFR_MAX_EAGER_ENTRIES) {
		dd_dev_err(dd,
			   "ctxt%u: requested too many RcvArray entries.\n",
			   context);
		rcd->eager_count = WFR_MAX_EAGER_ENTRIES;
	}
	if (rcd->eager_count % dd->rcv_entries.group_size) {
		/* eager_count is not a multiple of the group size */
		dd_dev_err(dd, "Eager count not multiple of group size (%u/%u)\n",
			   rcd->eager_count, dd->rcv_entries.group_size);
		ret = -EINVAL;
		goto done;
	}
	rcd->expected_count = max_entries - rcd->eager_count;
	if (rcd->expected_count > WFR_MAX_TID_PAIR_ENTRIES * 2)
		rcd->expected_count = WFR_MAX_TID_PAIR_ENTRIES * 2;

	rcd->eager_base = eager_base;
	rcd->expected_base = rcd->eager_base + rcd->eager_count;
	dd_dev_info(dd, "ctxt%u: eager:%u, exp:%u, egrbase:%u, expbase:%u\n",
		    context, rcd->eager_count, rcd->expected_count,
		    rcd->eager_base, rcd->expected_base);
done:
	return ret;
}

static int tempsense_rd(struct hfi_devdata *dd, int regnum)
{
	if (print_unimplemented)
		dd_dev_info(dd, "%s: not implemented\n", __func__);
	return -ENXIO;
}

/* ========================================================================= */

/*
 * Enable/disable chip from delivering interrupts.
 */
static void set_intr_state(struct hfi_devdata *dd, u32 enable)
{
	int i;

	/*
	 * In WFR, the mask needs to be 1 to allow interrupts.
	 */
	if (enable) {
		/* TODO: QIB_BADINTR check needed? */
		if (dd->flags & QIB_BADINTR)
			return;
		/* enable all interrupts */
		for (i = 0; i < WFR_CCE_NUM_INT_CSRS; i++)
			write_csr(dd, WFR_CCE_INT_MASK + (8*i), ~(u64)0);
		/*
		 * TODO: the 7322 wrote to INTCLEAR to "cause any
		 * pending interrupts to be redelivered".  The
		 * WFR HAS does not indicate that this this occurs
		 * for WFR.
		 *
		 * TODO: the 7322 also does a read and write of INTGRANTED
		 * for MSI-X interrupts.  Is there an equivalent for
		 * WFR?
		 * See qib_7322_set_intr_state() for details.
		 */

	} else {
		for (i = 0; i < WFR_CCE_NUM_INT_CSRS; i++)
			write_csr(dd, WFR_CCE_INT_MASK + (8*i), 0ull);
	}
}

/*
 * Clear all interrupt sources on the chip.
 */
static void clear_all_interrupts(struct hfi_devdata *dd)
{
	int i;

	for (i = 0; i < WFR_CCE_NUM_INT_CSRS; i++)
		write_csr(dd, WFR_CCE_INT_CLEAR + (8*i), ~(u64)0);

	write_csr(dd, WFR_CCE_ERR_CLEAR, ~(u64)0);
	write_csr(dd, WFR_MISC_ERR_CLEAR, ~(u64)0);
	write_csr(dd, WFR_RCV_ERR_CLEAR, ~(u64)0);
	write_csr(dd, WFR_SEND_ERR_CLEAR, ~(u64)0);
	write_csr(dd, WFR_SEND_PIO_ERR_CLEAR, ~(u64)0);
	write_csr(dd, WFR_SEND_DMA_ERR_CLEAR, ~(u64)0);
	write_csr(dd, WFR_SEND_EGRESS_ERR_CLEAR, ~(u64)0);
	for (i = 0; i < dd->chip_send_contexts; i++)
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_ERR_CLEAR, ~(u64)0);
	for (i = 0; i < dd->chip_sdma_engines; i++)
		write_kctxt_csr(dd, i, WFR_SEND_DMA_ENG_ERR_CLEAR, ~(u64)0);

	write_csr(dd, DCC_ERR_FLG_CLR, ~(u64)0);
	write_csr(dd, DC_LCB_ERR_CLR, ~(u64)0);
	write_csr(dd, DC_DC8051_ERR_CLR, ~(u64)0);
}

/* TODO: Move to pcie.c? */
static void disable_intx(struct pci_dev *pdev)
{
	pci_intx(pdev, 0);
}

static void clean_up_interrupts(struct hfi_devdata *dd)
{
	int i;

	/* remove irqs - must happen before disabling/turning off */
	if (dd->num_msix_entries) {
		/* MSI-X */
		struct qib_msix_entry *me = dd->msix_entries;
		for (i = 0; i < dd->num_msix_entries; i++, me++) {
			if (me->arg == NULL) /* => no irq, no affinity */
				break;
			irq_set_affinity_hint(dd->msix_entries[i].msix.vector,
					NULL);
			free_irq(me->msix.vector, me->arg);
		}
	} else {
		/* INTx */
		if (dd->requested_intx_irq) {
			free_irq(dd->pcidev->irq, dd);
			dd->requested_intx_irq = 0;
		}
	}

	/* turn off interrupts */
	if (dd->num_msix_entries) {
		/* MSI-X */
		qib_nomsix(dd);
	} else {
		/* INTx */
		disable_intx(dd->pcidev);
	}

	/* clean structures */
	for (i = 0; i < dd->num_msix_entries; i++)
		free_cpumask_var(dd->msix_entries[i].mask);
	kfree(dd->msix_entries);
	dd->msix_entries = NULL;
	dd->num_msix_entries = 0;
}

/*
 * Remap the interrupt source from the general handler to the given MSI-X
 * interrupt.
 */
static void remap_intr(struct hfi_devdata *dd, int isrc, int msix_intr)
{
	u64 reg;
	int m, n;

	/* clear from the handled mask of the general interrupt */
	m = isrc / 64;
	n = isrc % 64;
	dd->gi_mask[m] &= ~((u64)1 << n);

	/* direct the chip source to the given MSI-X interrupt */
	m = isrc / 8;
	n = isrc % 8;
	reg = read_csr(dd, WFR_CCE_INT_MAP + (8*m));
	reg &= ~((u64)0xff << (8*n));
	reg |= ((u64)msix_intr & 0xff) << (8*n);
	write_csr(dd, WFR_CCE_INT_MAP + (8*m), reg);
}

static void remap_sdma_interrupts(struct hfi_devdata *dd,
						int engine, int msix_intr)
{
	/*
	 * SDMA engine interrupt sources grouped by type, rather than
	 * engine.  Per-engine interrupts are as follows:
	 *	SDMA
	 *	SDMAProgress
	 *	SDMAIdle
	 */
	remap_intr(dd, WFR_IS_SDMA_START + 0*WFR_TXE_NUM_SDMA_ENGINES + engine,
		msix_intr);
	remap_intr(dd, WFR_IS_SDMA_START + 1*WFR_TXE_NUM_SDMA_ENGINES + engine,
		msix_intr);
	remap_intr(dd, WFR_IS_SDMA_START + 2*WFR_TXE_NUM_SDMA_ENGINES + engine,
		msix_intr);
}

static void remap_receive_available_interrupt(struct hfi_devdata *dd,
						int rx, int msix_intr)
{
	remap_intr(dd, WFR_IS_RCVAVAIL_START + rx, msix_intr);
}

static int request_intx_irq(struct hfi_devdata *dd)
{
	int ret;

	snprintf(dd->intx_name, sizeof(dd->intx_name), DRIVER_NAME"%d",
		dd->unit);
	ret = request_irq(dd->pcidev->irq, general_interrupt,
				  IRQF_SHARED, dd->intx_name, dd);
	if (ret)
		dd_dev_err(dd, "unable to request INTx interrupt, err %d\n",
				ret);
	else
		dd->requested_intx_irq = 1;
	return ret;
}

static int request_msix_irqs(struct hfi_devdata *dd)
{
	const struct cpumask *local_mask;
	int first_cpu, restart_cpu = 0, curr_cpu = 0;
	int local_node = pcibus_to_node(dd->pcidev->bus);
	int first_general, last_general;
	int first_sdma, last_sdma;
	int first_rx, last_rx;
	int i, ret;

	/* calculate the ranges we are going to use */
	first_general = 0;
	first_sdma = last_general = first_general + 1;
	first_rx = last_sdma = first_sdma + dd->num_sdma;
	last_rx = first_rx + dd->n_krcv_queues;

	/*
	 * Interrupt affinity.
	 *
	 * The "slow" interrupt can be shared with the rest of the
	 * interrupts clustered on the boot processor.  After
	 * that, distribute the rest of the "fast" interrupts
	 * on the remaining CPUs of the NUMA closest to the
	 * device.
	 *
	 * If on NUMA 0:
	 *	- place the slow interrupt on the first CPU
	 *	- distribute the rest, round robin, starting on
	 *	  the second CPU, avoiding cpu 0
	 *
	 * If not on NUMA 0:
	 *	- place the slow interrupt on the first CPU
	 *	- distribute the rest, round robin, including
	 *	  the first CPU
	 *
	 * Reasoning: If not on NUMA 0, then the first CPU
	 * does not have "everything else" on it and can
	 * be part of the interrupt distribution.
	 */
	local_mask = cpumask_of_pcibus(dd->pcidev->bus);
	first_cpu = cpumask_first(local_mask);
	/* TODO: What is the point of the cpumask_weight check? */
	if (first_cpu >= nr_cpu_ids ||
		cpumask_weight(local_mask) == num_online_cpus()) {
		local_mask = topology_core_cpumask(0);
		first_cpu = cpumask_first(local_mask);
	}
	if (first_cpu < nr_cpu_ids) {
		restart_cpu = cpumask_next(first_cpu, local_mask);
		if (restart_cpu >= nr_cpu_ids)
			restart_cpu = first_cpu;
	}
	/* decide the restart point */
	if (local_node > 0) {	/* not NUMA 0 */
		/* restart is the first */
		restart_cpu = first_cpu;
	}
	/*
	 * Start at the first cpu - we *know* the first
	 * interrupt is the slow interrupt.
	 */
	curr_cpu = first_cpu;

	/*
	 * Sanity check - the code expects all SDMA chip source
	 * interrupts to be in the same CSR, starting at bit 0.  Verify
	 * that this is true by checking the bit location of the start.
	 */
	if ((WFR_IS_SDMA_START % 64) != 0) {
		dd_dev_err(dd, "SDMA interrupt sources not CSR aligned");
		return -EINVAL;
	}

	for (i = 0; i < dd->num_msix_entries; i++) {
		struct qib_msix_entry *me = &dd->msix_entries[i];
		const char *err_info;
		irq_handler_t handler;
		void *arg;
		int idx;

		/*
		 * TODO:
		 * o Why isn't IRQF_SHARED used for the general interrupt
		 *   here?
		 * o Should we use IRQF_SHARED for non-NUMA 0?
		 * o If we truly wrapped, don't we need IRQF_SHARED?
		*/
		/* obtain the arguments to request_irq */
		if (first_general <= i && i < last_general) {
			idx = i - first_general;
			handler = general_interrupt;
			arg = dd;
			snprintf(me->name, sizeof(me->name),
				DRIVER_NAME"%d", dd->unit);
			err_info = "general";
		} else if (first_sdma <= i && i < last_sdma) {
			struct sdma_engine *per_sdma;
			idx = i - first_sdma;
			per_sdma = &dd->per_sdma[idx];
			/*
			 * Create a mask for all 3 chip interrupt sources
			 * mapped here.
			 */
			per_sdma->imask =
				  (u64)1 << (0*WFR_TXE_NUM_SDMA_ENGINES + idx)
				| (u64)1 << (1*WFR_TXE_NUM_SDMA_ENGINES + idx)
				| (u64)1 << (2*WFR_TXE_NUM_SDMA_ENGINES + idx);
			handler = sdma_interrupt;
			arg = per_sdma;
			snprintf(me->name, sizeof(me->name),
				DRIVER_NAME"%d sdma%d", dd->unit, idx);
			err_info = "sdma";
			remap_sdma_interrupts(dd, idx, i);
		} else if (first_rx <= i && i < last_rx) {
			struct qib_ctxtdata *rcd;
			idx = i - first_rx;
			rcd = dd->rcd[idx];
			/* no interrupt if no rcd */
			if (!rcd)
				continue;
			/*
			 * Set the interrupt register and mask for this
			 * context's interrupt.
			 */
			rcd->ireg = (WFR_IS_RCVAVAIL_START+idx) / 64;
			rcd->imask = ((u64)1) <<
					((WFR_IS_RCVAVAIL_START+idx) % 64);
			handler = receive_context_interrupt;
			arg = rcd;
			snprintf(me->name, sizeof(me->name),
				DRIVER_NAME"%d kctxt%d", dd->unit, idx);
			err_info = "receive context";
			remap_receive_available_interrupt(dd, idx, i);
		} else {
			/* not in our expected range - complain, then
			   ignore it */
			dd_dev_err(dd,
				"Unexpected extra MSI-X interrupt %d\n", i);
			continue;
		}
		/* no argument, no interrupt */
		if (arg == NULL)
			continue;
		/* make sure the name is terminated */
		me->name[sizeof(me->name)-1] = 0;

		ret = request_irq(me->msix.vector, handler, 0, me->name, arg);
		if (ret) {
			dd_dev_err(dd,
				"unable to allocate %s interrupt, vector %d, index %d, err %d\n",
				err_info, me->msix.vector, idx, ret);

			return ret;
		}
		/*
		 * assign arg after request_irq call, so it will be
		 * cleaned up
		 */
		me->arg = arg;

		/* set the affinity hint */
		if (first_cpu < nr_cpu_ids &&
			zalloc_cpumask_var(
				&dd->msix_entries[i].mask,
				GFP_KERNEL)) {
			cpumask_set_cpu(curr_cpu,
				dd->msix_entries[i].mask);
			curr_cpu = cpumask_next(curr_cpu, local_mask);
			if (curr_cpu >= nr_cpu_ids)
				curr_cpu = restart_cpu;
			irq_set_affinity_hint(
				dd->msix_entries[i].msix.vector,
				dd->msix_entries[i].mask);
		}
	}

	return 0;
}

/*
 * Set the general handler to accept all interrupts, remap all
 * chip interrupts back to MSI-X 0.
 */
static void reset_interrupts(struct hfi_devdata *dd)
{
	int i;

	/* all interrupts handled by the general handler */
	for (i = 0; i < WFR_CCE_NUM_INT_CSRS; i++)
		dd->gi_mask[i] = ~(u64)0;

	/* all chip interrupts map to MSI-X 0 */
	for (i = 0; i < WFR_CCE_NUM_INT_MAP_CSRS; i++)
		write_csr(dd, WFR_CCE_INT_MAP + (8*i), 0);
}

static int set_up_interrupts(struct hfi_devdata *dd)
{
	struct qib_msix_entry *entries;
	u32 total, request;
	int i, ret;
	int single_interrupt = 0; /* we expect to have all the interrupts */

	/*
	 * Interrupt count:
	 *	1 general, "slow path" interrupt (includes the SDMA engines
	 *		slow source, SDMACleanupDone)
	 *	N interrupts - one per used SDMA engine
	 *	M interrupt - one per kernel receive context
	 */
	total = 1 + dd->num_sdma + dd->n_krcv_queues;

	entries = kzalloc(sizeof(*entries) * total, GFP_KERNEL);
	if (!entries) {
		dd_dev_err(dd, "cannot allocate msix table\n");
		ret = -ENOMEM;
		goto fail;
	}
	/* 1-1 MSI-X entry assignment */
	for (i = 0; i < total; i++)
		entries[i].msix.entry = i;

	/* ask for MSI-X interrupts; expect a PCIe width of 16 */
	request = total;
	ret = qib_pcie_params(dd, 16, &request, entries);
	if (ret)
		goto fail;

	if (request == 0) {
		/* using INTx */
		/* dd->num_msix_entries already zero */
		kfree(entries);
		/* qib_pcie_params() will print if using INTx */
		single_interrupt = 1;
	} else {
		/* using MSI-X */
		dd->num_msix_entries = request;
		dd->msix_entries = entries;

		if (request != total) {
			/* using MSI-X, with reduced interrupts */
			/* TODO: Handle reduced interrupt case.  Need scheme to
			   decide who shares. */
			dd_dev_err(dd, "cannot handle reduced interrupt case"
				", want %u, got %u\n",
				total, request);
			ret = -EINVAL;
			goto fail;
		}
		dd_dev_info(dd, "%u MSI-X interrupts allocated\n", total);
	}

	/* mask all interrupts */
	set_intr_state(dd, 0);
	/* clear all pending interrupts */
	clear_all_interrupts(dd);

	/* reset general handler mask, chip MSI-X mappings */
	reset_interrupts(dd);

	/*
	 * Intialize per-SDMA data, so we have a unique pointer to hand to
	 * specialized handlers.
	 */
	for (i = 0; i < dd->num_sdma; i++) {
		dd->per_sdma[i].dd = dd;
		dd->per_sdma[i].which = i;
		spin_lock_init(&dd->per_sdma[i].senddmactrl_lock);
		dd->per_sdma[i].p_senddmactrl = 0; // JAG - probably wrong
	}

	if (single_interrupt)
		ret = request_intx_irq(dd);
	else
		ret = request_msix_irqs(dd);
	if (ret)
		goto fail;

	return 0;

fail:
	clean_up_interrupts(dd);
	return ret;
}

/*
 * Called from verify_interrupt() when it has detected that we have received
 * no interrupts.
 *
 * NOTE: The IRQ releases in clean_up_interrupts() require non-interrupt
 * context.  This means we can't be called from inside a timer function.
 */
static int intr_fallback(struct hfi_devdata *dd)
{
/*
 * TODO: remove #if when the simulation supports interrupts.
 * NOTE: the simulated HW does not support INTx yet, so we may want to
 *  keep the #if until then.
 */
#if 0
	if (dd->num_msix_entries == 0) {
		/* already using INTx.  Return a failure */
		 return 0;
	}
	/* clean our current set-up */
	clean_up_interrupts(dd);
	/* reset back to chip default */
	reset_interrupts(dd);
	/* set up INTx irq */
	request_intx_irq(dd);
	/* try again */
	return 1;
#else
	this_cpu_inc(*dd->int_counter);
	return 1;
#endif
}

/*
 * Set up context values in dd.  Sets:
 *
 *	num_rcv_contexts - number of contexts being used
 *	n_krcv_queues - number of kernel contexts
 *	first_user_ctxt - first non-kernel context in array of contexts
 *	freectxts  - nuber of free user contexts
 *	num_send_contexts - number of PIO send contexts being used
 */
static int set_up_context_variables(struct hfi_devdata *dd)
{
	int num_kernel_contexts;
	int num_user_contexts;
	int total_contexts;
	int ret;
	unsigned ngroups;

	/*
	 * Kernel contexts: (to be fixed later):
	 * 	- default to 1 kernel context per NUMA
	 */
	if (qib_n_krcv_queues) {
		num_kernel_contexts = qib_n_krcv_queues;
	} else {
		num_kernel_contexts = num_online_nodes();
	}

	/*
	 * User contexts: (to be fixed later)
	 *	- default to 1 user context per CPU
	 */
	num_user_contexts = num_online_cpus();

	total_contexts = num_kernel_contexts + num_user_contexts;

	/*
	 * Adjust the counts given a global max.
	 *	- always cut out user contexts before kernel contexts
	 *	- only extend user contexts
	 */
	if (num_rcv_contexts) {
		if (num_rcv_contexts < total_contexts) {
			/* cut back, user first */
			if (num_rcv_contexts < num_kernel_contexts) {
				num_kernel_contexts = num_rcv_contexts;
				num_user_contexts = 0;
			} else {
				num_user_contexts = num_rcv_contexts
							- num_kernel_contexts;
			}
		} else {
			/* extend the user context count */
			num_user_contexts = num_rcv_contexts
							- num_kernel_contexts;
		}
		/* recalculate */
		total_contexts = num_kernel_contexts + num_user_contexts;
	}

	if (total_contexts > dd->chip_rcv_contexts) {
		/* don't silently adjust, complain and fail */
		dd_dev_err(dd,
			"not enough physical contexts: want %d, have %d\n",
			(int)total_contexts, (int)dd->chip_rcv_contexts);
		return -ENOSPC;
	}

	/* the first N are kernel contexts, the rest are user contexts */
	dd->num_rcv_contexts = total_contexts;
	dd->n_krcv_queues = num_kernel_contexts;
	dd->first_user_ctxt = num_kernel_contexts;
	dd->freectxts = num_user_contexts;
	dd_dev_info(dd,
		"rcv contexts: chip %d, used %d (kernel %d, user %d)\n",
		(int)dd->chip_rcv_contexts,
		(int)dd->num_rcv_contexts,
		(int)dd->n_krcv_queues,
		(int)dd->num_rcv_contexts - dd->n_krcv_queues);

	/*
	 * Recieve array allocation:
	 *   All RcvArray entries are divided into groups of 8. This
	 *   is required by the hardware and will speed up writes to
	 *   consecutive entries by using write-combining of the entire
	 *   cacheline.
	 *
	 *   The number of groups are evenly divided among all contexts.
	 *   any left over groups will be given to the first N user
	 *   contexts.
	 */
	dd->rcv_entries.group_size = WFR_RCV_INCREMENT;
	ngroups = dd->chip_rcv_array_count / dd->rcv_entries.group_size;
	dd->rcv_entries.ngroups = ngroups / dd->num_rcv_contexts;
	dd->rcv_entries.nctxt_extra = ngroups -
		(dd->num_rcv_contexts * dd->rcv_entries.ngroups);
	dd_dev_info(dd, "RcvArray groups %u, ctxts extra %u\n",
		    dd->rcv_entries.ngroups,
		    dd->rcv_entries.nctxt_extra);
	if (dd->rcv_entries.ngroups * dd->rcv_entries.group_size >
	    WFR_MAX_EAGER_ENTRIES * 2) {
		dd->rcv_entries.ngroups = (WFR_MAX_EAGER_ENTRIES * 2) /
			dd->rcv_entries.group_size;
		dd_dev_info(dd,
		   "RcvArray group count too high, change to %u\n",
		   dd->rcv_entries.ngroups);
		dd->rcv_entries.nctxt_extra = 0;
	}
	/*
	 * PIO send contexts
	 */
	ret = init_sc_pools_and_sizes(dd);
	if (ret >= 0) {	/* success */
		dd->num_send_contexts = ret;
		dd_dev_info(dd, "send contexts: chip %d, used %d "
			"(kernel %d, ack %d, user %d)\n",
			dd->chip_send_contexts,
			dd->num_send_contexts,
			dd->sc_sizes[SC_KERNEL].count,
			dd->sc_sizes[SC_ACK].count,
			dd->sc_sizes[SC_USER].count);
		ret = 0;	/* success */
	}

	return ret;
}

/*
 * Set the device/port partition key table. The MAD code
 * will ensure that, at least, the partial management
 * partition key is present in the table.
 */
static void set_partition_keys(struct qib_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;
	u64 reg = 0;
	int i;
	dd_dev_info(dd, "Setting partition keys\n");
	for (i = 0; i < qib_get_npkeys(dd); i++) {
		reg |= (ppd->pkeys[i] &
			WFR_RCV_PARTITION_KEY_PARTITION_KEY_A_MASK) <<
			((i % 4) *
			 WFR_RCV_PARTITION_KEY_PARTITION_KEY_B_SHIFT);
		/* Each register holds 4 PKey values. */
		if ((i % 4) == 3) {
			write_csr(dd, WFR_RCV_PARTITION_KEY +
				  ((i - 3) * 2), reg);
			reg = 0;
		}
	}

	reg = read_csr(dd, WFR_RCV_CTRL);
	reg |= WFR_RCV_CTRL_RCV_PARTITION_KEY_ENABLE_SMASK;
	write_csr(dd, WFR_RCV_CTRL, reg);
}

/*
 * These CSRs and memories are uninitialized on reset and must be
 * written before reading to set the ECC/parity bits.
 *
 * NOTE: All user context CSRs that are not mmaped write-only
 * (e.g. the TID flows) must be initialized even if the driver never
 * reads them.
 */
static void write_uninitialized_csrs_and_memories(struct hfi_devdata *dd)
{
	int i, j;

	/* CceIntMap */
	for (i = 0; i < WFR_CCE_NUM_INT_MAP_CSRS; i++)
		write_csr(dd, WFR_CCE_INT_MAP+(8*i), 0);

	/* SendCtxtCreditReturnAddr */
	for (i = 0; i < dd->chip_send_contexts; i++)
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CREDIT_RETURN_ADDR, 0);

	/* PIO Send buffers */
	/* SDMA Send buffers */
	/* These are not normally read, and (presently) have no method
	   to be read, so are not pre-initialized */

	/* RcvHdrAddr */
	/* RcvHdrTailAddr */
	/* RcvTidFlowTable */
	for (i = 0; i < dd->chip_rcv_contexts; i++) {
		write_kctxt_csr(dd, i, WFR_RCV_HDR_ADDR, 0);
		write_kctxt_csr(dd, i, WFR_RCV_HDR_TAIL_ADDR, 0);
		for (j = 0; j < WFR_RXE_NUM_TID_FLOWS; j++)
			write_uctxt_csr(dd, i, WFR_RCV_TID_FLOW_TABLE+(8*j), 0);
	}

	/* RcvArray */
	for (i = 0; i < dd->chip_rcv_array_count; i++)
		write_csr(dd, WFR_RCV_ARRAY + (8*i),
					WFR_RCV_ARRAY_RT_WRITE_ENABLE_SMASK);

	/* RcvQPMapTable */
	for (i = 0; i < 32; i++)
		write_csr(dd, WFR_RCV_QP_MAP_TABLE + (8 * i), 0);
}

/* set CCE CSRs to chip reset defaults */
static void reset_cce_csrs(struct hfi_devdata *dd)
{
	int i;

	/* WFR_CCE_REVISION read-only */
	/* WFR_CCE_REVISION2 read-only */
	write_csr(dd, WFR_CCE_CTRL, read_csr(dd, WFR_CCE_CTRL));
	/* WFR_CCE_STATUS read-only */
	for (i = 0; i < CCE_NUM_SCRATCH; i++)
		write_csr(dd, WFR_CCE_SCRATCH + (8 * i), 0);
	/* WFR_CCE_ERR_STATUS read-only */
	write_csr(dd, WFR_CCE_ERR_MASK, 0);
	write_csr(dd, WFR_CCE_ERR_CLEAR, ~0ull);
	/* WFR_CCE_ERR_FORCE leave alone */
	for (i = 0; i < CCE_NUM_32_BIT_COUNTERS; i++)
		write_csr(dd, WFR_CCE_COUNTER_ARRAY32 + (8 * i), 0);
	/* TODO: what is the policy for CceDbiCtrl? */
	/* TODO: WFR_CCE_DBI_CTRL */
	/* TODO: WFR_CCE_DBI_ADDR */
	/* TODO: WFR_CCE_DBI_DATA */
	write_csr(dd, WFR_CCE_DC_CTRL, WFR_CCE_DC_CTRL_RESETCSR);
	/* TODO: what is the policy for CcePcieCtrl? Leave alone? */
	/* TODO: write_csr(dd, WFR_CCE_PCIE_CTRL, 0);*/
	for (i = 0; i < CCE_NUM_MSIX_VECTORS; i++) {
		write_csr(dd, WFR_CCE_MSIX_TABLE_LOWER + (8 * i), 0);
		write_csr(dd, WFR_CCE_MSIX_TABLE_UPPER + (8 * i),
					WFR_CCE_MSIX_TABLE_UPPER_RESETCSR);
	}
	for (i = 0; i < CCE_NUM_MSIX_PBAS; i++) {
		/* WFR_CCE_MSIX_PBA read-only */
		write_csr(dd, WFR_CCE_MSIX_INT_GRANTED, ~0ull);
		write_csr(dd, WFR_CCE_MSIX_VEC_CLR_WITHOUT_INT, ~0ull);
	}
	for (i = 0; i < CCE_NUM_INT_MAP_CSRS; i++)
		write_csr(dd, WFR_CCE_INT_MAP, 0);
	for (i = 0; i < CCE_NUM_INT_CSRS; i++) {
		/* WFR_CCE_INT_STATUS read-only */
		write_csr(dd, WFR_CCE_INT_MASK + (8 * i), 0);
		write_csr(dd, WFR_CCE_INT_CLEAR + (8 * i), ~0ull);
		/* WFR_CCE_INT_FORCE leave alone */
		/* WFR_CCE_INT_BLOCKED read-only */
	}
	for (i = 0; i < CCE_NUM_32_BIT_INT_COUNTERS; i++)
		write_csr(dd, WFR_CCE_INT_COUNTER_ARRAY32 + (8 * i), 0);
}

/* set ASIC CSRs to chip reset defaults */
static void reset_asic_csrs(struct hfi_devdata *dd)
{
	static DEFINE_MUTEX(asic_mutex);
	static int called;
	int i;

	/*
	 * TODO:  If the HFIs are shared between separate nodes or VMs,
	 * then more will need to be done here.  One idea is a module
	 * parameter that returns early, letting the first power-on or
	 * a known first load do the reset and blocking all others.
	 */

	/*
	 * These CSRs should only be reset once - the first one here will
	 * do the work.  Use a mutex so that a non-first caller waits until
	 * the first is finished before it can proceed.
	 */
	mutex_lock(&asic_mutex);
	if (called)
		goto done;
	called = 1;

	if (dd->icode != WFR_ICODE_FPGA_EMULATION) {
		/* emulation does not have an SBUS - leave these alone */
		/*
		 * TODO: All writes to ASIC_CFG_SBUS_REQUEST do something.
		 * Do we want to write a reset here or leave it alone?
		 * Notes:
		 * o The reset is not zero if aimed at the core.  See the
		 *   SBUS documentation for details.
		 * o If the SBUS firmware has been upated (e.g. by the BIOS),
		 *   will the reset revert that?
		 */
		/*write_csr(dd, WFR_ASIC_CFG_SBUS_REQUEST, 0);*/
		write_csr(dd, WFR_ASIC_CFG_SBUS_EXECUTE, 0);
	}
	/* WFR_ASIC_SBUS_RESULT read-only */
	write_csr(dd, WFR_ASIC_STS_SBUS_COUNTERS, 0);
	for (i = 0; i < ASIC_NUM_SCRATCH; i++)
		write_csr(dd, WFR_ASIC_CFG_SCRATCH + (8 * i), 0);
	write_csr(dd, WFR_ASIC_CFG_MUTEX, 0);	/* this will clear it */
	write_csr(dd, WFR_ASIC_CFG_DRV_STR, 0);
	write_csr(dd, WFR_ASIC_CFG_THERM_POLL_EN, 0);
	/* WFR_ASIC_STS_THERM read-only */
	/* WFR_ASIC_CFG_RESET leave alone */

	write_csr(dd, WFR_ASIC_PCIE_SD_HOST_CMD, 0);
	/* WFR_ASIC_PCIE_SD_HOST_STATUS read-only */
	write_csr(dd, WFR_ASIC_PCIE_SD_INTRPT_DATA_CODE, 0);
	write_csr(dd, WFR_ASIC_PCIE_SD_INTRPT_ENABLE, 0);
	/* WFR_ASIC_PCIE_SD_INTRPT_PROGRESS read-only */
	write_csr(dd, WFR_ASIC_PCIE_SD_INTRPT_STATUS, ~0ull); /* clear */
	/* WFR_ASIC_HFI0_PCIE_SD_INTRPT_RSPD_DATA read-only */
	/* WFR_ASIC_HFI1_PCIE_SD_INTRPT_RSPD_DATA read-only */
	for (i = 0; i < 16; i++)
		write_csr(dd, WFR_ASIC_PCIE_SD_INTRPT_LIST + (8 * i), 0);

	/* WFR_ASIC_GPIO_IN read-only */
	write_csr(dd, WFR_ASIC_GPIO_OE, 0);
	write_csr(dd, WFR_ASIC_GPIO_INVERT, 0);
	write_csr(dd, WFR_ASIC_GPIO_OUT, 0);
	write_csr(dd, WFR_ASIC_GPIO_MASK, 0);
	/* WFR_ASIC_GPIO_STATUS read-only */
	write_csr(dd, WFR_ASIC_GPIO_CLEAR, ~0ull);
	/* WFR_ASIC_GPIO_FORCE leave alone */

	/* WFR_ASIC_QSFP1_IN read-only */
	write_csr(dd, WFR_ASIC_QSFP1_OE, 0);
	write_csr(dd, WFR_ASIC_QSFP1_INVERT, 0);
	write_csr(dd, WFR_ASIC_QSFP1_OUT, 0);
	write_csr(dd, WFR_ASIC_QSFP1_MASK, 0);
	/* WFR_ASIC_QSFP1_STATUS read-only */
	write_csr(dd, WFR_ASIC_QSFP1_CLEAR, ~0ull);
	/* WFR_ASIC_QSFP1_FORCE leave alone */

	/* WFR_ASIC_QSFP2_IN read-only */
	write_csr(dd, WFR_ASIC_QSFP2_OE, 0);
	write_csr(dd, WFR_ASIC_QSFP2_INVERT, 0);
	write_csr(dd, WFR_ASIC_QSFP2_OUT, 0);
	write_csr(dd, WFR_ASIC_QSFP2_MASK, 0);
	/* WFR_ASIC_QSFP2_STATUS read-only */
	write_csr(dd, WFR_ASIC_QSFP2_CLEAR, ~0ull);
	/* WFR_ASIC_QSFP2_FORCE leave alone */

	write_csr(dd, WFR_ASIC_EEP_CTL_STAT, WFR_ASIC_EEP_CTL_STAT_RESETCSR);
	/* this also writes a NOP command, clearing paging mode */
	write_csr(dd, WFR_ASIC_EEP_ADDR_CMD, 0);
	write_csr(dd, WFR_ASIC_EEP_DATA, 0);
	/* WFR_ASIC_MAN_EFUSE* read-only */
	/* WFR_ASIC_WFR_EFUSE read-only */
	/* WFR_ASIC_WFR_EFUSE_REGS* read-only */

done:
	mutex_unlock(&asic_mutex);
}

/* set MISC CSRs to chip reset defaults */
static void reset_misc_csrs(struct hfi_devdata *dd)
{
	int i;

	for (i = 0; i < 32; i++) {
		write_csr(dd, WFR_MISC_CFG_RSA_R2 + (8 * i), 0);
		write_csr(dd, WFR_MISC_CFG_RSA_SIGNATURE + (8 * i), 0);
		/*
		 * TODO: Writing this causes the PCIe to fail in emulation.
		 * Note: It is not required that these RSA arrays be zeroed
		 * as they all need to be completely written to work correctly.
		 */
		/*write_csr(dd, WFR_MISC_CFG_RSA_MODULUS + (8 * i), 0);*/
	}
	write_csr(dd, WFR_MISC_CFG_SHA_PRELOAD, 0);
	write_csr(dd, WFR_MISC_CFG_RSA_CMD, 0);
	write_csr(dd, WFR_MISC_CFG_RSA_MU, 0);
	write_csr(dd, WFR_MISC_CFG_FW_CTRL, 0);
	/* WFR_MISC_STS_8051_DIGEST read-only */
	/* WFR_MISC_STS_SBM_DIGEST read-only */
	/* WFR_MISC_STS_PCIE_DIGEST read-only */
	/* WFR_MISC_STS_FAB_DIGEST read-only */
	/* WFR_MISC_ERR_STATUS read-only */
	write_csr(dd, WFR_MISC_ERR_MASK, 0);
	write_csr(dd, WFR_MISC_ERR_CLEAR, ~0ull);
	/* WFR_MISC_ERR_FORCE leave alone */
}

/* set TXE CSRs to chip reset defaults */
static void reset_txe_csrs(struct hfi_devdata *dd)
{
	int i;

	/*
	 * TXE Kernel CSRs
	 */
	write_csr(dd, WFR_SEND_CTRL, 0);
	__cm_reset(dd, 0);	/* reset CM internal state */
	/* WFR_SEND_CONTEXTS read-only */
	/* WFR_SEND_DMA_ENGINES read-only */
	/* WFR_SEND_PIO_MEM_SIZE read-only */
	/* WFR_SEND_DMA_MEM_SIZE read-only */
	write_csr(dd, WFR_SEND_HIGH_PRIORITY_LIMIT, 0);
	pio_reset_all(dd);	/* WFR_SEND_PIO_INIT_CTXT */
	/* WFR_SEND_PIO_ERR_STATUS read-only */
	write_csr(dd, WFR_SEND_PIO_ERR_MASK, 0);
	write_csr(dd, WFR_SEND_PIO_ERR_CLEAR, ~0ull);
	/* WFR_SEND_PIO_ERR_FORCE leave alone */
	/* WFR_SEND_DMA_ERR_STATUS read-only */
	write_csr(dd, WFR_SEND_DMA_ERR_MASK, 0);
	write_csr(dd, WFR_SEND_DMA_ERR_CLEAR, ~0ull);
	/* WFR_SEND_DMA_ERR_FORCE leave alone */
	/* WFR_SEND_EGRESS_ERR_STATUS read-only */
	write_csr(dd, WFR_SEND_EGRESS_ERR_MASK, 0);
	write_csr(dd, WFR_SEND_EGRESS_ERR_CLEAR, ~0ull);
	/* WFR_SEND_EGRESS_ERR_FORCE leave alone */
	write_csr(dd, WFR_SEND_BTH_QP, 0);
	write_csr(dd, WFR_SEND_STATIC_RATE_CONTROL, 0);
	write_csr(dd, WFR_SEND_SC2VLT0, 0);
	write_csr(dd, WFR_SEND_SC2VLT1, 0);
	write_csr(dd, WFR_SEND_SC2VLT2, 0);
	write_csr(dd, WFR_SEND_SC2VLT3, 0);
	write_csr(dd, WFR_SEND_LEN_CHECK0, 0);
	write_csr(dd, WFR_SEND_LEN_CHECK1, 0);
	/* WFR_SEND_ERR_STATUS read-only */
	write_csr(dd, WFR_SEND_ERR_MASK, 0);
	write_csr(dd, WFR_SEND_ERR_CLEAR, ~0ull);
	/* WFR_SEND_ERR_FORCE read-only */
	for (i = 0; i < WFR_VL_ARB_LOW_PRIO_TABLE_SIZE; i++)
		write_csr(dd, WFR_SEND_LOW_PRIORITY_LIST + (8*i), 0);
	for (i = 0; i < WFR_VL_ARB_HIGH_PRIO_TABLE_SIZE; i++)
		write_csr(dd, WFR_SEND_HIGH_PRIORITY_LIST + (8*i), 0);
	for (i = 0; i < dd->chip_send_contexts/WFR_NUM_CONTEXTS_PER_SET; i++)
		write_csr(dd, WFR_SEND_CONTEXT_SET_CTRL + (8*i), 0);
	for (i = 0; i < WFR_TXE_NUM_32_BIT_COUNTER; i++)
		write_csr(dd, WFR_SEND_COUNTER_ARRAY32 + (8*i), 0);
	for (i = 0; i < WFR_TXE_NUM_64_BIT_COUNTER; i++)
		write_csr(dd, WFR_SEND_COUNTER_ARRAY64 + (8*i), 0);
	write_csr(dd, WFR_SEND_CM_CTRL, WFR_SEND_CM_CTRL_RESETCSR);
	write_csr(dd, WFR_SEND_CM_GLOBAL_CREDIT,
					WFR_SEND_CM_GLOBAL_CREDIT_RESETCSR);
	/* WFR_SEND_CM_CREDIT_USED_STATUS read-only */
	write_csr(dd, WFR_SEND_CM_TIMER_CTRL, 0);
	write_csr(dd, WFR_SEND_CM_LOCAL_AU_TABLE0_TO3, 0);
	write_csr(dd, WFR_SEND_CM_LOCAL_AU_TABLE4_TO7, 0);
	write_csr(dd, WFR_SEND_CM_REMOTE_AU_TABLE0_TO3, 0);
	write_csr(dd, WFR_SEND_CM_REMOTE_AU_TABLE4_TO7, 0);
	for (i = 0; i < WFR_TXE_NUM_DATA_VL; i++)
		write_csr(dd, WFR_SEND_CM_CREDIT_VL + (8*i), 0);
	write_csr(dd, WFR_SEND_CM_CREDIT_VL15, 0);
	/* WFR_SEND_CM_CREDIT_USED_VL read-only */
	/* WFR_SEND_CM_CREDIT_USED_VL15 read-only */
	/* WFR_SEND_EGRESS_CTXT_STATUS read-only */
	/* WFR_SEND_EGRESS_SEND_DMA_STATUS read-only */
	write_csr(dd, WFR_SEND_EGRESS_ERR_INFO, ~0ull);
	/* WFR_SEND_EGRESS_ERR_INFO read-only */
	/* WFR_SEND_EGRESS_ERR_SOURCE read-only */

	/*
	 * TXE Per-Context CSRs
	 */
	for (i = 0; i < dd->chip_send_contexts; i++) {
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CTRL, 0);
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CREDIT_CTRL, 0);
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CREDIT_RETURN_ADDR, 0);
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CREDIT_FORCE, 0);
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_ERR_MASK, 0);
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_ERR_CLEAR, ~0ull);
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CHECK_ENABLE, 0);
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CHECK_VL, 0);
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CHECK_JOB_KEY, 0);
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CHECK_PARTITION_KEY, 0);
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CHECK_SLID, 0);
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CHECK_OPCODE, 0);
	}

	/*
	 * TXE Per-SDMA CSRs
	 */
	for (i = 0; i < dd->chip_sdma_engines; i++) {
		write_kctxt_csr(dd, i, WFR_SEND_DMA_CTRL, 0);
		/* WFR_SEND_DMA_STATUS read-only */
		write_kctxt_csr(dd, i, WFR_SEND_DMA_BASE_ADDR, 0);
		write_kctxt_csr(dd, i, WFR_SEND_DMA_LEN_GEN, 0);
		write_kctxt_csr(dd, i, WFR_SEND_DMA_TAIL, 0);
		/* WFR_SEND_DMA_HEAD read-only */
		write_kctxt_csr(dd, i, WFR_SEND_DMA_HEAD_ADDR, 0);
		write_kctxt_csr(dd, i, WFR_SEND_DMA_PRIORITY_THLD, 0);
		/* WFR_SEND_DMA_IDLE_CNT read-only */
		write_kctxt_csr(dd, i, WFR_SEND_DMA_RELOAD_CNT, 0);
		write_kctxt_csr(dd, i, WFR_SEND_DMA_DESC_CNT, 0);
		/* WFR_SEND_DMA_DESC_FETCHED_CNT read-only */
		/* WFR_SEND_DMA_ENG_ERR_STATUS read-only */
		write_kctxt_csr(dd, i, WFR_SEND_DMA_ENG_ERR_MASK, 0);
		write_kctxt_csr(dd, i, WFR_SEND_DMA_ENG_ERR_CLEAR, ~0ull);
		/* WFR_SEND_DMA_ENG_ERR_FORCE leave alone */
		write_kctxt_csr(dd, i, WFR_SEND_DMA_CHECK_ENABLE, 0);
		write_kctxt_csr(dd, i, WFR_SEND_DMA_CHECK_VL, 0);
		write_kctxt_csr(dd, i, WFR_SEND_DMA_CHECK_JOB_KEY, 0);
		write_kctxt_csr(dd, i, WFR_SEND_DMA_CHECK_PARTITION_KEY, 0);
		write_kctxt_csr(dd, i, WFR_SEND_DMA_CHECK_SLID, 0);
		write_kctxt_csr(dd, i, WFR_SEND_DMA_CHECK_OPCODE, 0);
		write_kctxt_csr(dd, i, WFR_SEND_DMA_MEMORY, 0);
	}
}

/*
 * Expect on entry:
 * o Packet ingress is disabled, i.e. RcvCtrl.RcvPortEnable == 0
 */
static void init_rbufs(struct hfi_devdata *dd)
{
	u64 reg;
	int count;

	/*
	 * Wait for DMA to stop: RxRbufPktPending and RxPktInProgress are
	 * clear.
	 */
	count = 0;
	while (1) {
		reg = read_csr(dd, WFR_RCV_STATUS);
		if ((reg & (WFR_RCV_STATUS_RX_RBUF_PKT_PENDING_SMASK
			    | WFR_RCV_STATUS_RX_PKT_IN_PROGRESS_SMASK)) == 0)
			break;
		/*
		 * Give up after 1ms - maximum wait time.
		 *
		 * RBuf size is 148KiB.  Slowest possible is PCIe Gen1 x1 at
		 * 250MB/s bandwidth.  Derate that at 66% for overhead to get:
		 *	148 KB / (66% * 250MB/s) = 920us
		 */
		if (count++ > 500) {
			dd_dev_err(dd,
				"%s: in-progress DMA not clearing: RcvStatus 0x%llx, continuing\n",
				__func__, reg);
			break;
		}
		udelay(2); /* do not busy-wait the CSR */
	}

	/* start the init */
	write_csr(dd, WFR_RCV_CTRL,
		read_csr(dd, WFR_RCV_CTRL) | WFR_RCV_CTRL_RX_RBUF_INIT_SMASK);
	/*
	 * Read to force the write of Rcvtrl.RxRbufInit.  There is a brief
	 * period after the write before RcvStatus.RxRbufInitDone is valid.
	 * The delay in the first run through the loop below is sufficient and
	 * required before the first read of RcvStatus.RxRbufInintDone.
	 */
	read_csr(dd, WFR_RCV_CTRL);

	/* wait for the init to finish */
	count = 0;
	while (1) {
		/* delay is required first time through - see above */
		udelay(2); /* do not busy-wait the CSR */
		reg = read_csr(dd, WFR_RCV_STATUS);
		if (reg & (WFR_RCV_STATUS_RX_RBUF_INIT_DONE_SMASK))
			break;

		/* give up after 100us - slowest possible at 33MHz is 73us */
		if (count++ > 50) {
			dd_dev_err(dd,
				"%s: RcvStatus.RxRbufInit not set, continuing\n",
				__func__);
			break;
		}
	}
}

/* set RXE CSRs to chip reset defaults */
static void reset_rxe_csrs(struct hfi_devdata *dd)
{
	int i, j;

	/*
	 * RXE Kernel CSRs
	 */
	write_csr(dd, WFR_RCV_CTRL, 0);
	init_rbufs(dd);
	/* WFR_RCV_STATUS read-only */
	/* WFR_RCV_CONTEXTS read-only */
	/* WFR_RCV_ARRAY_CNT read-only */
	/* WFR_RCV_BUF_SIZE read-only */
	write_csr(dd, WFR_RCV_BTH_QP, 0);
	write_csr(dd, WFR_RCV_MULTICAST, 0);
	write_csr(dd, WFR_RCV_BYPASS, 0);
	write_csr(dd, WFR_RCV_VL15, 0);
	/* this is a clear-down */
	write_csr(dd, WFR_RCV_ERR_INFO,
			WFR_RCV_ERR_INFO_RCV_EXCESS_BUFFER_OVERRUN_SMASK);
	/* WFR_RCV_ERR_STATUS read-only */
	write_csr(dd, WFR_RCV_ERR_MASK, 0);
	write_csr(dd, WFR_RCV_ERR_CLEAR, ~0ull);
	/* WFR_RCV_ERR_FORCE leave alone */
	for (i = 0; i < 32; i++)
		write_csr(dd, WFR_RCV_QP_MAP_TABLE + (8 * i), 0);
	for (i = 0; i < 4; i++)
		write_csr(dd, WFR_RCV_PARTITION_KEY + (8 * i), 0);
	for (i = 0; i < RXE_NUM_32_BIT_COUNTERS; i++)
		write_csr(dd, WFR_RCV_COUNTER_ARRAY32 + (8 * i), 0);
	for (i = 0; i < RXE_NUM_64_BIT_COUNTERS; i++)
		write_csr(dd, WFR_RCV_COUNTER_ARRAY64 + (8 * i), 0);
	for (i = 0; i < RXE_NUM_RSM_INSTANCES; i++) {
		write_csr(dd, WFR_RCV_RSM_CFG + (8 * i), 0);
		write_csr(dd, WFR_RCV_RSM_SELECT + (8 * i), 0);
		write_csr(dd, WFR_RCV_RSM_MATCH + (8 * i), 0);
	}
	for (i = 0; i < 32; i++)
		write_csr(dd, WFR_RCV_RSM_MAP_TABLE + (8 * i), 0);

	/*
	 * RXE Kernel and User Per-Context CSRs
	 */
	for (i = 0; i < dd->chip_rcv_contexts; i++) {
		/* kernel */
		write_kctxt_csr(dd, i, WFR_RCV_CTXT_CTRL, 0);
		/* WFR_RCV_CTXT_STATUS read-only */
		write_kctxt_csr(dd, i, WFR_RCV_EGR_CTRL, 0);
		write_kctxt_csr(dd, i, WFR_RCV_TID_CTRL, 0);
		write_kctxt_csr(dd, i, WFR_RCV_KEY_CTRL, 0);
		write_kctxt_csr(dd, i, WFR_RCV_HDR_ADDR, 0);
		write_kctxt_csr(dd, i, WFR_RCV_HDR_CNT, 0);
		write_kctxt_csr(dd, i, WFR_RCV_HDR_ENT_SIZE, 0);
		write_kctxt_csr(dd, i, WFR_RCV_HDR_SIZE, 0);
		write_kctxt_csr(dd, i, WFR_RCV_HDR_TAIL_ADDR, 0);
		write_kctxt_csr(dd, i, WFR_RCV_AVAIL_TIME_OUT, 0);
		write_kctxt_csr(dd, i, WFR_RCV_HDR_OVFL_CNT, 0);

		/* user */
		/* WFR_RCV_HDR_TAIL read-only */
		write_uctxt_csr(dd, i, WFR_RCV_HDR_HEAD, 0);
		/* WFR_RCV_EGR_INDEX_TAIL read-only */
		write_uctxt_csr(dd, i, WFR_RCV_EGR_INDEX_HEAD, 0);
		/* WFR_RCV_EGR_OFFSET_TAIL read-only */
		for (j = 0; j < RXE_NUM_TID_FLOWS; j++) {
			write_uctxt_csr(dd, i, WFR_RCV_TID_FLOW_TABLE + (8 * j),
				0);
		}
	}
}

/*
 * Set sc2vl tables.
 *
 * They power on to zeros, so to avoid send context errors
 * they need to be set:
 *
 * SC 0-7 -> VL 0-7 (respectively)
 * SC 15  -> VL 15
 * otherwize
 *        -> VL 0
 */
void init_sc2vl_tables(struct hfi_devdata *dd)
{
	/* init per architecture spec, contrained by hardware capability */

	/* WFR maps sent packets */
	write_csr(dd, WFR_SEND_SC2VLT0, SC2VL_VAL(
		0,
		0, 0, 1, 1,
		2, 2, 3, 3,
		4, 4, 5, 5,
		6, 6, 7, 7));
	write_csr(dd, WFR_SEND_SC2VLT1, SC2VL_VAL(
		1,
		8, 0, 9, 0,
		10, 0, 11, 0,
		12, 0, 13, 0,
		14, 0, 15, 15));
	write_csr(dd, WFR_SEND_SC2VLT2, SC2VL_VAL(
		2,
		16, 0, 17, 0,
		18, 0, 19, 0,
		20, 0, 21, 0,
		22, 0, 23, 0));
	write_csr(dd, WFR_SEND_SC2VLT3, SC2VL_VAL(
		3,
		24, 0, 25, 0,
		26, 0, 27, 0,
		28, 0, 29, 0,
		30, 0, 31, 0));

#ifdef WFR_SEND_CM_GLOBAL_CREDIT_SHARED_LIMIT_SHIFT
/* TODO: only do this with WFR HAS 0.76 (sim v33) and later, the sim changed
   what it does with these CSRs at that time */
	/* DC maps received packets */
	write_csr(dd, DCC_CFG_SC_VL_TABLE_15_0, DC_SC_VL_VAL(
		15_0,
		0, 0, 1, 1,  2, 2,  3, 3,  4, 4,  5, 5,  6, 6,  7,  7,
		8, 0, 9, 0, 10, 0, 11, 0, 12, 0, 13, 0, 14, 0, 15, 15));
	write_csr(dd, DCC_CFG_SC_VL_TABLE_31_16, DC_SC_VL_VAL(
		31_16,
		16, 0, 17, 0, 18, 0, 19, 0, 20, 0, 21, 0, 22, 0, 23, 0,
		24, 0, 25, 0, 26, 0, 27, 0, 28, 0, 29, 0, 30, 0, 31, 0));
#endif
}

/*
 * Read chip sizes and then reset parts to sane, disabled, values.  We cannot
 * depend on the chip just being reset - a driver may be loaded and
 * unloaded many times.
 *
 * Sets:
 * dd->chip_rcv_contexts  - number of receive contexts supported by the chip
 * dd->chip_rcv_array_count - number of entries in the Receive Array
 * dd->chip_send_contexts - number PIO send contexts supported by the chip
 * dd->chip_pio_mem_size  - size of PIO send memory, in blocks
 * dd->chip_sdma_engines  - number of SDMA engines supported by the chip
 * dd->vau		  - encoding of AU as AU = 8 * 2^vAU
 * dd->link_credits	  - number of link credits available
 */
static void init_chip(struct hfi_devdata *dd)
{
	int i;

	dd->chip_rcv_contexts = read_csr(dd, WFR_RCV_CONTEXTS);
	dd->chip_send_contexts = read_csr(dd, WFR_SEND_CONTEXTS);
	dd->chip_sdma_engines = read_csr(dd, WFR_SEND_DMA_ENGINES);
	dd->chip_pio_mem_size = read_csr(dd, WFR_SEND_PIO_MEM_SIZE);
	/* FPGA workaround: this CSR may not contain the correct value */
	if (dd->icode == WFR_ICODE_FPGA_EMULATION
						&& dd->chip_sdma_engines != 4) {
		dd_dev_info(dd, "WORKAROUND: forcing sdma engines to 4\n");
		dd->chip_sdma_engines = 4;
	}

	/*
	 * Put the WFR CSRs in a known state.
	 * Combine this with a DC reset.
	 *
	 * Stop the device from doing anything while we do a
	 * reset.  We know there are no other active users of
	 * the device since we are now in charge.  Turn off
	 * off all outbound and inbound traffic and make sure
	 * the device does not generate any interrupts.
	 */

	/* disable send contexts and SDMA engines */
	write_csr(dd, WFR_SEND_CTRL, 0);
	for (i = 0; i < dd->chip_send_contexts; i++)
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CTRL, 0);
	for (i = 0; i < dd->chip_sdma_engines; i++)
		write_kctxt_csr(dd, i, WFR_SEND_DMA_CTRL, 0);
	/* disable port (turn off RXE inbound traffic) and contexts */
	write_csr(dd, WFR_RCV_CTRL, 0);
	for (i = 0; i < dd->chip_rcv_contexts; i++)
		write_csr(dd, WFR_RCV_CTXT_CTRL, 0);
	/* mask all interrupt sources */
	for (i = 0; i < WFR_CCE_NUM_INT_CSRS; i++)
		write_csr(dd, WFR_CCE_INT_MASK + (8*i), 0ull);

	/*
	 * DC Reset: do a full DC reset before the register clear.
	 * A recommended length of time to hold is one CSR read,
	 * so reread the CceDcCtrl.  Then, hold the DC in reset
	 * across the clear.
	 */
	write_csr(dd, WFR_CCE_DC_CTRL, WFR_CCE_DC_CTRL_DC_RESET_SMASK);
	(void) read_csr(dd, WFR_CCE_DC_CTRL);

	if (use_flr) {
		/*
		 * A FLR will reset the SPC core and part of the PCIe.
		 * TODO: This wipes out the PCIe BARs - anything else
		 *`	in PCIe?
		 */
		dd_dev_info(dd, "Resetting CSRs with FLR\n");

		/* do the FLR, the DC reset will remain */
		hfi_pcie_flr(dd);

	} else {
		dd_dev_info(dd, "Resetting CSRs with writes\n");
		reset_cce_csrs(dd);
		reset_txe_csrs(dd);
		reset_rxe_csrs(dd);
		reset_asic_csrs(dd);
		reset_misc_csrs(dd);
	}
	/* clear the DC reset */
	write_csr(dd, WFR_CCE_DC_CTRL, 0);

	/* assign link credit variables */
	dd->vau = WFR_CM_VAU;
	dd->link_credits = WFR_CM_GLOBAL_CREDITS;
	dd->vcu = cu_to_vcu(hfi_cu);
	/* TODO: Make initial VL15 credit size a parameter? */
	/* enough room for 8 MAD packets plus header - 17K */
	dd->vl15_init = (8 * (2048 + 128)) / vau_to_au(dd->vau);
	if (dd->vl15_init > dd->link_credits)
		dd->vl15_init = dd->link_credits;

	write_uninitialized_csrs_and_memories(dd);

	if (enable_pkeys)
		for (i = 0; i < dd->num_pports; i++) {
			struct qib_pportdata *ppd = &dd->pport[i];
			set_partition_keys(ppd);
		}
	init_sc2vl_tables(dd);

	/*
	 * TODO: The following block is strictly for the simulator.
	 * If we find that the physical and logical states are LinkUp
	 * and Active respectively, then assume we are in the simulator's
	 * "EasyLinkup" mode.  When in EayLinkup mode, we don't change
	 * the DC's physical and logical states.
	 */
	if (dd->icode == WFR_ICODE_FUNCTIONAL_SIMULATOR) {
		u32 ps = read_physical_state(dd);
		u32 ls = __read_logical_state(dd);
		if (ps == WFR_PLS_LINKUP && ls == WFR_LSTATE_ACTIVE) {
			/* assume we're in the simulator's easy linkup mode */
			if (sim_easy_linkup == EASY_LINKUP_UNSET
							|| sim_easy_linkup == 1) {
				sim_easy_linkup = 1;
				dd_dev_info(dd, "Assuming simulation EasyLinkup mode\n");
			} else { 
				dd_dev_info(dd, "This HFI is in EasyLinkup mode, but the other is not?\n");
			}
		} else {
			sim_easy_linkup = 0;
			if (sim_easy_linkup == EASY_LINKUP_UNSET
							|| sim_easy_linkup == 0) {
				sim_easy_linkup = 0;
			} else { 
				dd_dev_info(dd, "This HFI is in normal link mode, but the other is not?\n");
			}
			/* otherwise the link should be offline with port down */
			if (!(ps == WFR_PLS_OFFLINE && ls == WFR_LSTATE_DOWN)) {
				__print_current_states(dd, __func__, "at start", ps, ls);
				set_physical_link_state(dd, WFR_PLS_OFFLINE);
				dd_dev_err(dd, "Manually setting physical link to offline - not supposed to be necessary\n");
				print_current_states(dd, __func__, "after force offline");
			}
		}
	} else {
		sim_easy_linkup = 0;
	}
}

static void init_kdeth_qp(struct hfi_devdata *dd)
{
	/* user changed the KDETH_QP */
	if (kdeth_qp != 0 && kdeth_qp >= 0xff) {
		/* out of range or illegal value */
		dd_dev_err(dd, "Invalid KDETH queue pair prefix, ignoring");
		kdeth_qp = 0;
	}
	if (kdeth_qp == 0)	/* not set, or failed range check */
		kdeth_qp = DEFAULT_KDETH_QP;

	write_csr(dd, WFR_SEND_BTH_QP,
			(kdeth_qp & WFR_SEND_BTH_QP_KDETH_QP_MASK)
				<< WFR_SEND_BTH_QP_KDETH_QP_SHIFT);

	write_csr(dd, WFR_RCV_BTH_QP,
			(kdeth_qp & WFR_RCV_BTH_QP_KDETH_QP_MASK)
				<< WFR_RCV_BTH_QP_KDETH_QP_SHIFT);
}

/**
 * init_qpmap_table
 * @dd - device data
 * @regno - first register in 32 register series
 * @first_ctxt - first context
 * @last_ctxt - first context
 *
 * This return sets the qpn mapping table that
 * sitting being qpn[8:1].
 *
 * The routine will round robin the 256 settings
 * from first_ctxt to last_ctxt.
 *
 * The first/last looks ahead to having specialized
 * receive contexts for mgmt and bypass.  Normal
 * verbs traffic will assumed to be on a range
 * of receive contexts.
 */
static void init_qpmap_table(struct hfi_devdata *dd,
			     u32 first_ctxt,
			     u32 last_ctxt)
{
	u64 reg = 0;
	u64 regno = WFR_RCV_QP_MAP_TABLE;
	int i;
	u64 ctxt = first_ctxt;

	for (i = 0; i < 256; ) {
		reg |= ctxt << (8 * (i % 8));
		i++;
		ctxt++;
		if (ctxt > last_ctxt)
			ctxt = first_ctxt;
		if (i % 8 == 0) {
			write_csr(dd, regno, reg);
			reg = 0;
			regno += 8;
		}
	}
	if (i % 8)
		write_csr(dd, regno, reg);
}

void init_rxe(struct hfi_devdata *dd)
{
	u64 reg;
	/* enable all receive errors */
	write_csr(dd, WFR_RCV_ERR_MASK, ~0ull);
	/* setup QPN map table */
	init_qpmap_table(dd, 0, dd->n_krcv_queues - 1);
	/* enable map table */
	reg = read_csr(dd, WFR_RCV_CTRL);
	reg |= WFR_RCV_CTRL_RCV_QP_MAP_ENABLE_SMASK;
	write_csr(dd, WFR_RCV_CTRL, reg);

	/* TODO: others...? */
}

void init_other(struct hfi_devdata *dd)
{
	/* enable all CCE errors */
	write_csr(dd, WFR_CCE_ERR_MASK, ~0ull);
	/* enable all Misc errors */
	write_csr(dd, WFR_MISC_ERR_MASK, ~0ull);
	/* enable all DC errors */
	write_csr(dd, DCC_ERR_FLG_EN, ~0ull);
	write_csr(dd, DC_LCB_ERR_EN, ~0ull);
	write_csr(dd, DC_DC8051_ERR_EN, ~0ull);
}

/*
 * Fill out the given AU table using the given CU.  A CU is defined in terms
 * AUs.  The table is a an encoding: given the index, how many AUs does that
 * represent?
 *
 * NOTE: Assumes that the register layout is the same for the
 * local and remote tables.
 */
static void assign_cm_au_table(struct hfi_devdata *dd, u32 cu,
						u32 csr0to3, u32 csr4to7)
{
	write_csr(dd, csr0to3,
		   0ull <<
			WFR_SEND_CM_LOCAL_AU_TABLE0_TO3_LOCAL_AU_TABLE0_SHIFT
		|  1ull <<
			WFR_SEND_CM_LOCAL_AU_TABLE0_TO3_LOCAL_AU_TABLE1_SHIFT
		|  2ull * cu <<
			WFR_SEND_CM_LOCAL_AU_TABLE0_TO3_LOCAL_AU_TABLE2_SHIFT
		|  4ull * cu <<
			WFR_SEND_CM_LOCAL_AU_TABLE0_TO3_LOCAL_AU_TABLE3_SHIFT);
	write_csr(dd, csr4to7,
		   8ull * cu <<
			WFR_SEND_CM_LOCAL_AU_TABLE4_TO7_LOCAL_AU_TABLE4_SHIFT
		| 16ull * cu <<
			WFR_SEND_CM_LOCAL_AU_TABLE4_TO7_LOCAL_AU_TABLE5_SHIFT
		| 32ull * cu <<
			WFR_SEND_CM_LOCAL_AU_TABLE4_TO7_LOCAL_AU_TABLE6_SHIFT
		| 64ull * cu <<
			WFR_SEND_CM_LOCAL_AU_TABLE4_TO7_LOCAL_AU_TABLE7_SHIFT);

}

static void assign_local_cm_au_table(struct hfi_devdata *dd, u8 vcu)
{
	assign_cm_au_table(dd, vcu_to_cu(vcu), WFR_SEND_CM_LOCAL_AU_TABLE0_TO3,
					WFR_SEND_CM_LOCAL_AU_TABLE4_TO7);
}

void assign_remote_cm_au_table(struct hfi_devdata *dd, u8 vcu)
{
	assign_cm_au_table(dd, vcu_to_cu(vcu), WFR_SEND_CM_REMOTE_AU_TABLE0_TO3,
					WFR_SEND_CM_REMOTE_AU_TABLE4_TO7);
}

void init_txe(struct hfi_devdata *dd)
{
	int i;

	/* enable all PIO, SDMA, general, and Egress errors */
	write_csr(dd, WFR_SEND_PIO_ERR_MASK, ~0ull);
	write_csr(dd, WFR_SEND_DMA_ERR_MASK, ~0ull);
	write_csr(dd, WFR_SEND_ERR_MASK, ~0ull);
	write_csr(dd, WFR_SEND_EGRESS_ERR_MASK, ~0ull);

	/* enable all per-context and per-SDMA engine errors */
	for (i = 0; i < dd->chip_send_contexts; i++)
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_ERR_MASK, ~0ull);
	for (i = 0; i < dd->chip_sdma_engines; i++)
		write_kctxt_csr(dd, i, WFR_SEND_DMA_ENG_ERR_MASK, ~0ull);

	/* set the local CU to AU mapping */
	assign_local_cm_au_table(dd, dd->vcu);
}

int set_ctxt_jkey(struct hfi_devdata *dd, unsigned ctxt, u16 jkey)
{
	struct qib_ctxtdata *rcd = dd->rcd[ctxt];
	unsigned sctxt;
	int ret = 0;
	u64 reg;

	if (!rcd || !rcd->sc) {
		ret = -EINVAL;
		goto done;
	}
	sctxt = rcd->sc->context;
	reg = WFR_SEND_CTXT_CHECK_JOB_KEY_MASK_SMASK | /* mask is always 1's */
		((jkey & WFR_SEND_CTXT_CHECK_JOB_KEY_VALUE_MASK) <<
		 WFR_SEND_CTXT_CHECK_JOB_KEY_VALUE_SHIFT);
	/* JOB_KEY_ALLOW_PERMISSIVE is not allowed by default */
	if (rcd->flags & HFI_CTXTFLAG_ALLOWPERMJKEY)
		reg |= WFR_SEND_CTXT_CHECK_JOB_KEY_ALLOW_PERMISSIVE_SMASK;
	write_kctxt_csr(dd, sctxt, WFR_SEND_CTXT_CHECK_JOB_KEY, reg);
	/* Turn on the J_KEY check */
	reg = read_kctxt_csr(dd, sctxt, WFR_SEND_CTXT_CHECK_ENABLE);
	reg |= WFR_SEND_CTXT_CHECK_ENABLE_CHECK_JOB_KEY_SMASK;
	write_kctxt_csr(dd, sctxt, WFR_SEND_CTXT_CHECK_ENABLE, reg);

	/* Enable J_KEY check on receive context. */
	reg = WFR_RCV_KEY_CTRL_JOB_KEY_ENABLE_SMASK |
		((jkey & WFR_RCV_KEY_CTRL_JOB_KEY_VALUE_MASK) <<
		 WFR_RCV_KEY_CTRL_JOB_KEY_VALUE_SHIFT);
	write_kctxt_csr(dd, ctxt, WFR_RCV_KEY_CTRL, reg);
done:
	return ret;
}

int clear_ctxt_jkey(struct hfi_devdata *dd, unsigned ctxt)
{
	struct qib_ctxtdata *rcd = dd->rcd[ctxt];
	unsigned sctxt;
	int ret = 0;
	u64 reg;

	if (!rcd || !rcd->sc) {
		ret = -EINVAL;
		goto done;
	}
	sctxt = rcd->sc->context;
	write_kctxt_csr(dd, sctxt, WFR_SEND_CTXT_CHECK_JOB_KEY, 0);
	/* Turn off the J_KEY check on the send context */
	reg = read_kctxt_csr(dd, sctxt, WFR_SEND_CTXT_CHECK_ENABLE);
	reg &= ~WFR_SEND_CTXT_CHECK_ENABLE_CHECK_JOB_KEY_SMASK;
	write_kctxt_csr(dd, sctxt, WFR_SEND_CTXT_CHECK_ENABLE, reg);
	/* Turn off the J_KEY on the receive side */
	write_kctxt_csr(dd, ctxt, WFR_RCV_KEY_CTRL, 0);
done:
	return ret;
}

int set_ctxt_pkey(struct hfi_devdata *dd, unsigned ctxt, u16 pkey)
{
	struct qib_ctxtdata *rcd;
	unsigned sctxt;
	int ret = 0;
	u64 reg;

	if (ctxt < dd->num_rcv_contexts)
		rcd = dd->rcd[ctxt];
	else {
		ret = -EINVAL;
		goto done;
	}
	if (!rcd || !rcd->sc) {
		ret = -EINVAL;
		goto done;
	}
	sctxt = rcd->sc->context;
	reg = ((u64)pkey & WFR_SEND_CTXT_CHECK_PARTITION_KEY_VALUE_MASK) <<
		WFR_SEND_CTXT_CHECK_PARTITION_KEY_VALUE_SHIFT;
	write_kctxt_csr(dd, sctxt, WFR_SEND_CTXT_CHECK_PARTITION_KEY, reg);
	reg = read_kctxt_csr(dd, sctxt, WFR_SEND_CTXT_CHECK_ENABLE);
	reg |= WFR_SEND_CTXT_CHECK_ENABLE_CHECK_PARTITION_KEY_SMASK;
	write_kctxt_csr(dd, sctxt, WFR_SEND_CTXT_CHECK_ENABLE, reg);
done:
	return ret;
}

int clear_ctxt_pkey(struct hfi_devdata *dd, unsigned ctxt)
{
	struct qib_ctxtdata *rcd;
	unsigned sctxt;
	int ret = 0;
	u64 reg;

	if (ctxt < dd->num_rcv_contexts)
		rcd = dd->rcd[ctxt];
	else {
		ret = -EINVAL;
		goto done;
	}
	if (!rcd || !rcd->sc) {
		ret = -EINVAL;
		goto done;
	}
	sctxt = rcd->sc->context;
	reg = read_kctxt_csr(dd, sctxt, WFR_SEND_CTXT_CHECK_ENABLE);
	reg &= ~WFR_SEND_CTXT_CHECK_ENABLE_CHECK_PARTITION_KEY_SMASK;
	write_kctxt_csr(dd, sctxt, WFR_SEND_CTXT_CHECK_ENABLE, reg);
	write_kctxt_csr(dd, sctxt, WFR_SEND_CTXT_CHECK_PARTITION_KEY, 0);
done:
	return ret;
}

/*
 * Assign link credits.
 *
 * This routine is called to assign per-VL link credits.  This is done
 * when requested via module parameter, generally because there is not
 * an STL compliant FM running.
 *
 * Assumptions:
 * 	- the link partner has same credits as WFR 
 *
 * The assignment methodology is arbitrary.  Details:
 * o place half of the credits into the per-VL dedicated limit slots
 * o total dedicated = sum of per-VL and VL15
 * o total shared = global - total dedicated
 * All normal VLs will use the global shared values.  VL15 will not
 * use any share credits.
 */
void assign_link_credits(struct hfi_devdata *dd)
{
	struct buffer_control t;
	int i;

	memset(&t, 0, sizeof(struct buffer_control));

/* enough room for 2 full-sized MAD packets plus max headers */
#define VL15_CREDITS (((MAX_MAD_PACKET + 128) * 2)/64)

#define PER_VL_DEDICATED_CREDITS \
	(WFR_CM_GLOBAL_CREDITS / (2 * WFR_TXE_NUM_DATA_VL))
#define TOTAL_DEDICATED_CREDITS \
	((PER_VL_DEDICATED_CREDITS * WFR_TXE_NUM_DATA_VL) + VL15_CREDITS)
#define TOTAL_SHARED_CREDITS (WFR_CM_GLOBAL_CREDITS - TOTAL_DEDICATED_CREDITS)
#define PER_VL_SHARED_CREDITS TOTAL_SHARED_CREDITS

	BUG_ON(WFR_CM_GLOBAL_CREDITS < TOTAL_DEDICATED_CREDITS);
	t.overall_shared_limit = cpu_to_be16(TOTAL_SHARED_CREDITS);
	for (i = 0; i < WFR_TXE_NUM_DATA_VL; i++) {
		t.vl[i].dedicated = cpu_to_be16(PER_VL_DEDICATED_CREDITS);
		t.vl[i].shared = cpu_to_be16(PER_VL_SHARED_CREDITS);
	}
	t.vl[15].dedicated = cpu_to_be16(VL15_CREDITS);

	set_buffer_control(dd, &t);
}

/*
 * Clean up stuff initialized in qib_init_wfr_funcs() - mostly.
 * TODO: Cleanup of stuff done in qib_init_wfr_funcs() gets done elsewhere,
 * so we have an asymmetric cleanup.
 *
 * This is f_cleanup
 * TODO:
 * - Remove indirect call
 * - rename to chip_cleanup() or asic_cleanup()
 * - rename qib_init_wfr_funcs() to chip_init() or asic_init()
 *
 * called from qib_postinit_cleanup()
 */
static void cleanup(struct hfi_devdata *dd)
{
	free_cntrs(dd);
	clean_up_interrupts(dd);
}

/**
 * qib_init_wfr_funcs - set up the chip-specific function pointers
 * @dev: the pci_dev for qlogic_ib device
 * @ent: pci_device_id struct for this dev
 *
 * Also allocates, inits, and returns the devdata struct for this
 * device instance
 *
 * This is global, and is called directly at init to set up the
 * chip-specific function pointers for later use.
 */
struct hfi_devdata *qib_init_wfr_funcs(struct pci_dev *pdev,
					   const struct pci_device_id *ent)
{
	struct hfi_devdata *dd;
	struct qib_pportdata *ppd;
	u64 rev2;
	int i, ret;
	static const char *inames[] = { /* implementation names */
		"RTL silicon",
		"RTL VCS simulation",
		"RTL FPGA emulation",
		"Functional simulator"
	};

	dd = qib_alloc_devdata(pdev,
		NUM_IB_PORTS * sizeof(struct qib_pportdata));
	if (IS_ERR(dd))
		goto bail;
	ppd = dd->pport;
	for (i = 0; i < dd->num_pports; i++, ppd++) {
		int vl;
		/* init common fields */
		qib_init_pportdata(ppd, dd, 0, 1);
		/* chip specific */
#ifndef CONFIG_STL_MGMT
		ppd->link_speed_supported = IB_SPEED_EDR;
		ppd->link_width_supported = IB_WIDTH_1X | IB_WIDTH_4X;
#else
		/* from DC HAS */
		ppd->link_speed_supported =
			STL_LINK_SPEED_25G | STL_LINK_SPEED_12_5G;
		/* widths build on IB - adds 2,3 */
		ppd->link_width_supported =
			IB_WIDTH_1X | IB_WIDTH_4X |
			STL_LINK_WIDTH_2X | STL_LINK_WIDTH_3X;
#endif
		ppd->link_speed_enabled = ppd[i].link_speed_supported;

		switch (num_vls) {
		case 1:
			ppd->vls_supported = IB_VL_VL0;
			break;
		case 2:
			ppd->vls_supported = IB_VL_VL0_1;
			break;
		default:
			dd_dev_info(dd,
				    "Invalid num_vls %u, using 4 VLs\n",
				    num_vls);
			num_vls = 4;
			/* fall through */
		case 4:
			ppd->vls_supported = IB_VL_VL0_3;
			break;
		case 8:
			ppd->vls_supported = IB_VL_VL0_7;
			break;
		}
		ppd->vls_operational = ppd->vls_supported;
		/* Set the default MTU. */
		for (vl = 0; vl < num_vls; vl++)
			dd->vld[vl].mtu = default_mtu;
		dd->vld[15].mtu = MAX_MAD_PACKET;
		/*
		 * Set the initial values to reasonable default, will be set
		 * for real when link is up.
		 */
#ifndef CONFIG_STL_MGMT
		ppd->link_speed_active = IB_SPEED_EDR;
#else
		ppd->link_speed_active = STL_LINK_SPEED_25G;
#endif
		ppd->link_width_active = IB_WIDTH_4X;
		ppd->lstate = IB_PORT_DOWN;
		ppd->overrun_threshold = 0x4;
		ppd->phy_error_threshold = 0xf;
	}

	dd->f_bringup_serdes    = bringup_serdes;
	dd->f_cleanup           = cleanup;
	dd->f_clear_tids        = clear_tids;
	dd->f_free_irq          = stop_irq;
	dd->f_get_base_info     = get_base_info;
	dd->f_get_msgheader     = get_msgheader;
	dd->f_gpio_mod          = gpio_mod;
	dd->f_hdrqempty         = hdrqempty;
	dd->f_init_ctxt         = init_ctxt;
	dd->f_intr_fallback     = intr_fallback;
	dd->f_portcntr          = portcntr;
	dd->f_put_tid           = put_tid;
	dd->f_quiet_serdes      = quiet_serdes;
	dd->f_rcvctrl           = rcvctrl;
	dd->f_read_cntrs        = read_cntrs;
	dd->f_read_portcntrs    = read_portcntrs;
	dd->f_reset             = reset;
	dd->f_init_sdma_regs    = init_sdma_regs;
	dd->f_sdma_busy         = sdma_busy;
	dd->f_sdma_gethead      = sdma_gethead;
	dd->f_sdma_sendctrl     = sdma_sendctrl;
	dd->f_sdma_set_desc_cnt = sdma_set_desc_cnt;
	dd->f_sdma_update_tail  = sdma_update_tail;
	dd->f_set_armlaunch     = set_armlaunch;
	dd->f_set_cntr_sample   = set_cntr_sample;
	dd->f_iblink_state      = iblink_state;
	dd->f_ibphys_portstate  = ibphys_portstate;
	dd->f_get_ib_cfg        = get_ib_cfg;
	dd->f_set_ib_cfg        = set_ib_cfg;
	dd->f_set_ib_loopback   = set_ib_loopback;
	dd->f_set_intr_state    = set_intr_state;
	dd->f_setextled         = setextled;
	dd->f_update_usrhead    = update_usrhead;
	dd->f_wantpiobuf_intr   = sc_wantpiobuf_intr;
	dd->f_xgxs_reset        = xgxs_reset;
	dd->f_sdma_hw_clean_up  = sdma_hw_clean_up;
	dd->f_sdma_hw_start_up  = sdma_hw_start_up;
	dd->f_sdma_init_early   = sdma_init_early;
	dd->f_tempsense_rd	= tempsense_rd;
	dd->f_set_ctxt_jkey     = set_ctxt_jkey;
	dd->f_clear_ctxt_jkey   = clear_ctxt_jkey;
	dd->f_set_ctxt_pkey     = set_ctxt_pkey;
	dd->f_clear_ctxt_pkey   = clear_ctxt_pkey;

	/*
	 * Set other early dd values.
	 */
	dd->link_default = IB_LINKINITCMD_POLL;

	/*
	 * Do remaining PCIe setup and save PCIe values in dd.
	 * Any error printing is already done by the init code.
	 * On return, we have the chip mapped.
	 */
	ret = qib_pcie_ddinit(dd, pdev, ent);
	if (ret < 0)
		goto bail_free;

	/* verify that reads actually work, save revision for reset check */
	dd->revision = read_csr(dd, WFR_CCE_REVISION);
	if (dd->revision == ~(u64)0) {
		dd_dev_err(dd, "cannot read chip CSRs\n");
		ret = -EINVAL;
		goto bail_cleanup;
	}
	dd->majrev = (dd->revision >> WFR_CCE_REVISION_CHIP_REV_MAJOR_SHIFT)
			& WFR_CCE_REVISION_CHIP_REV_MAJOR_MASK;
	dd->minrev = (dd->revision >> WFR_CCE_REVISION_CHIP_REV_MINOR_SHIFT)
			& WFR_CCE_REVISION_CHIP_REV_MINOR_MASK;

	/* obtain the hardware ID - NOT related to unit, which is a
	   software enumeration */
	rev2 = read_csr(dd, WFR_CCE_REVISION2);
	dd->hfi_id = (rev2 >> WFR_CCE_REVISION2_HFI_ID_SHIFT)
					& WFR_CCE_REVISION2_HFI_ID_MASK;
	/* the variable size will remove unwanted bits */
	dd->icode = rev2 >> WFR_CCE_REVISION2_IMPL_CODE_SHIFT;
	dd->irev = rev2 >> WFR_CCE_REVISION2_IMPL_REVISION_SHIFT;
	dd_dev_info(dd, "Implementation: %s, revision 0x%x\n",
		dd->icode < ARRAY_SIZE(inames) ? inames[dd->icode] : "unknown",
		(int)dd->irev);

	/* cannot run on older revisions */
	if (dd->icode == WFR_ICODE_FUNCTIONAL_SIMULATOR && dd->irev < 43) {
		dd_dev_err(dd, "Version mismatch: This driver requires simulator version 43 or higher\n");
		ret = -EINVAL;
		goto bail_cleanup;
	}
	if (dd->icode == WFR_ICODE_FPGA_EMULATION && dd->irev < 0x1503) {
		dd_dev_err(dd, "Version mismatch: This driver requires emulation version 0x15 or higher\n");
		ret = -EINVAL;
		goto bail_cleanup;
	}


	/* obtain chip sizes, reset chip CSRs */
	init_chip(dd);

#if 0
	//TODO: What should be done, if anything, about this?
	// SL Select: - only used for 9B packets
	// Values:
	//	0 = SC (service channel)
	//	1 = SL (service level) - SC lookup based on the SL of
	//				 the packet header
	write_csr(dd, DCC_DCC_CFG_PORT_CONFIG,
			read_csr(dd, DCC_DCC_CFG_PORT_CONFIG)
				| DCC_DCC_CFG_PORT_CONFIG_SL_SELECT_MODE_SMASK);
#endif

	/* sdma init */
	if (use_sdma)
		dd->flags |= QIB_HAS_SEND_DMA;
	dd->flags |= sdma_idle_cnt ? QIB_HAS_SDMA_TIMEOUT : 0;
	dd->num_sdma = dd->chip_sdma_engines;
	if (mod_num_sdma && mod_num_sdma < dd->chip_sdma_engines)
		dd->num_sdma = mod_num_sdma;

dd_dev_err(dd, "JAG SDMA %s:%d %s()\n", __FILE__, __LINE__, __func__);
dd_dev_err(dd, "JAG SDMA mod_num_sdma: %u\n", mod_num_sdma);
dd_dev_err(dd, "JAG SDMA dd->chip_sdma_engines: %u\n", dd->chip_sdma_engines);
dd_dev_err(dd, "JAG SDMA dd->num_sdma: %u\n", dd->num_sdma);

	if (nodma_rtail)
		dd->flags |= QIB_NODMA_RTAIL;

	dd->palign = 0x1000;	// TODO: is there a WFR value for this?


	/* TODO these are set by the chip and don't change */
	/* per-context kernel/user CSRs */
	dd->uregbase = WFR_RXE_PER_CONTEXT_USER;
	dd->ureg_align = WFR_RXE_PER_CONTEXT_SIZE;

	/*
	 * The receive eager buffer size must be set before the receive
	 * contexts are created.
	 *
	 * Set the eager bufer size.  Validate that it falls in a range
	 * allowed by the hardware - all powers of 2 between the min and
	 * max.  The maximum valid MTU is within the eager buffer range
	 * so we do not need to cap the max_mtu by an eager buffer size
	 * setting.
	 */
	dd->rcvegrbufsize = eager_buffer_size ? eager_buffer_size : max_mtu;
	if (dd->rcvegrbufsize < WFR_MIN_EAGER_BUFFER)
		dd->rcvegrbufsize = WFR_MIN_EAGER_BUFFER;
	if (dd->rcvegrbufsize > WFR_MAX_EAGER_BUFFER)
		dd->rcvegrbufsize = WFR_MAX_EAGER_BUFFER;
	dd->rcvegrbufsize = __roundup_pow_of_two(dd->rcvegrbufsize);
	dd->rcvegrbufsize_shift = ilog2(dd->rcvegrbufsize);

	/* TODO: real board name */
	dd->boardname = kmalloc(64, GFP_KERNEL);
	sprintf(dd->boardname, "fake wfr");

	/* TODO: RcvHdrEntSize, RcvHdrCnt, and RcvHdrSize are now
	   per context, rather than global. */
	/* FIXME: arbitrary/old values */
	dd->rcvhdrcnt = 64;
	/* TODO: make rcvhdrentsize and rcvhdrsize adjustable? */
	dd->rcvhdrentsize = DEFAULT_RCVHDR_ENTSIZE;
	dd->rcvhdrsize = DEFAULT_RCVHDRSIZE;
	dd->rhf_offset = dd->rcvhdrentsize - sizeof(u64) / sizeof(u32);

	ret = set_up_context_variables(dd);
	if (ret)
		goto bail_cleanup;

	/* set initial RXE CSRs */
	init_rxe(dd);
	/* set initial TXE CSRs */
	init_txe(dd);
	/* set initial non-RXE, non-TXE CSRs */
	init_other(dd);
	/* set up KDETH QP prefix in both RX and TX CSRs */
	init_kdeth_qp(dd);

	/* send contexts must be set up before receive contexts */
	ret = init_send_contexts(dd);
	if (ret)
		goto bail_cleanup;

	ret = init_pervl_scs(dd);
	if (ret)
		goto bail_cleanup;

	ret = qib_create_ctxts(dd);
	if (ret)
		goto bail_cleanup;

	/* use contexts created by qib_create_ctxts */
	ret = set_up_interrupts(dd);
	if (ret)
		goto bail_cleanup;

	read_guid(dd);
	ret = load_firmware(dd); /* asymmetric with dispose_firmware() */
	if (ret)
		goto bail_clear_intr;

	ret = init_cntrs(dd);
	if (ret)
		goto bail_clear_intr;

	goto bail;

bail_clear_intr:
	clean_up_interrupts(dd);
bail_cleanup:
	qib_pcie_ddcleanup(dd);
bail_free:
	qib_free_devdata(dd);
	dd = ERR_PTR(ret);
bail:
	return dd;
}

/**
 * create_pbc - build a pbc for transmission
 * @flags: special case flags or-ed in built pbc
 * @srate: static rate
 * @vl: vl
 * @dwlen: dwork length
 *
 * Create a PBC with the given flags, rate, VL, and length.
 *
 * NOTE: The PBC created will not insert any HCRC - all callers but one are
 * for verbs, which does not use this PSM feature.  The lone other caller
 * is for the diagnostic interface which calls this if the user does not
 * supply their own PBC.
 */
u64 create_pbc(u64 flags, u32 srate, u32 vl, u32 dw_len)
{
	u64 pbc;
	/* FIXME: calculate rate */
	u32 rate = 0;

	pbc = flags
		| ((u64)WFR_PBC_IHCRC_NONE << WFR_PBC_INSERT_HCRC_SHIFT)
		| (vl & WFR_PBC_VL_MASK) << WFR_PBC_VL_SHIFT
		| (rate & WFR_PBC_STATIC_RATE_CONTROL_COUNT_MASK)
			<< WFR_PBC_STATIC_RATE_CONTROL_COUNT_SHIFT
		| (dw_len & WFR_PBC_LENGTH_DWS_MASK)
			<< WFR_PBC_LENGTH_DWS_SHIFT;

	return pbc;
}

/* interrupt testing */
static void force_errors(struct hfi_devdata *dd, u32 csr, const char *what)
{
	int i;

	for (i = 0; i < 64; i++) {
		dd_dev_info(dd, "** Forced interrupt: %s %d\n", what, i);
		write_csr(dd, csr, 1ull << i);
		msleep(100);
	}
}

void force_all_interrupts(struct hfi_devdata *dd)
{
	int i, j;
	char buf[64];

	for (i = 0; i < WFR_CCE_NUM_INT_CSRS; i++) {
		for (j = 0; j < 64; j++) {
			dd_dev_info(dd, "** Forced interrupt: "
				"csr #%2d, bit %2d; \"%s\"\n",
				i, j, is_name(buf, sizeof(buf), (i*64)+j));
			write_csr(dd, WFR_CCE_INT_FORCE + (8*i), 1ull << j);
			/*
			 * We want this delay so it is long enough that
			 * the interrupt prints match the print above
			 * but no so long that it takes forever to run.
			 */
			/*ssleep(1);*/
			msleep(100);
		}
	}

	force_errors(dd, WFR_CCE_ERR_FORCE, "CCE Err");
	force_errors(dd, WFR_RCV_ERR_FORCE, "Receive Err");
	force_errors(dd, WFR_SEND_PIO_ERR_FORCE, "Send PIO Err");
	force_errors(dd, WFR_SEND_DMA_ERR_FORCE, "Send DMA Err");
	force_errors(dd, WFR_SEND_EGRESS_ERR_FORCE, "Send Egress Err");
	force_errors(dd, WFR_SEND_ERR_FORCE, "Send Err");
}

#define sdma_dumpstate_helper(reg) do { \
		csr = read_kctxt_csr(dd, 0, reg); \
		dd_dev_err(dd, "%36s 0x%016llx\n", #reg, csr); \
	} while ( 0 )

#define sdma_dumpstate_helper2(reg) do { \
		csr = read_csr(dd, reg + (8 * i)); \
		dd_dev_err(dd, "%33s_%02u 0x%016llx\n", #reg, i, csr); \
	} while ( 0 )

#ifdef JAG_SDMA_VERBOSITY
void qib_sdma0_dumpstate(struct hfi_devdata *dd)
{
	u64 csr;
	unsigned i;

	sdma_dumpstate_helper(WFR_SEND_DMA_CTRL);
	sdma_dumpstate_helper(WFR_SEND_DMA_STATUS);
	sdma_dumpstate_helper(WFR_SEND_DMA_ERR_STATUS);
	sdma_dumpstate_helper(WFR_SEND_DMA_ERR_MASK);
	sdma_dumpstate_helper(WFR_SEND_DMA_ENG_ERR_STATUS);
	sdma_dumpstate_helper(WFR_SEND_DMA_ENG_ERR_MASK);

	for (i = 0; i < WFR_CCE_NUM_INT_CSRS; ++i ) {
		sdma_dumpstate_helper2(WFR_CCE_INT_STATUS);
		sdma_dumpstate_helper2(WFR_CCE_INT_MASK);
		sdma_dumpstate_helper2(WFR_CCE_INT_BLOCKED);
	}

	sdma_dumpstate_helper(WFR_SEND_DMA_TAIL);
	sdma_dumpstate_helper(WFR_SEND_DMA_HEAD);
	sdma_dumpstate_helper(WFR_SEND_DMA_PRIORITY_THLD);
	sdma_dumpstate_helper(WFR_SEND_DMA_IDLE_CNT);
	sdma_dumpstate_helper(WFR_SEND_DMA_RELOAD_CNT);
	sdma_dumpstate_helper(WFR_SEND_DMA_DESC_CNT);
	sdma_dumpstate_helper(WFR_SEND_DMA_DESC_FETCHED_CNT);
	sdma_dumpstate_helper(WFR_SEND_DMA_MEMORY);
	sdma_dumpstate_helper(WFR_SEND_DMA_ENGINES);
	sdma_dumpstate_helper(WFR_SEND_DMA_MEM_SIZE);
	sdma_dumpstate_helper(WFR_SEND_EGRESS_SEND_DMA_STATUS);
	sdma_dumpstate_helper(WFR_SEND_DMA_BASE_ADDR);
	sdma_dumpstate_helper(WFR_SEND_DMA_LEN_GEN);
	sdma_dumpstate_helper(WFR_SEND_DMA_HEAD_ADDR);
	sdma_dumpstate_helper(WFR_SEND_DMA_CHECK_ENABLE);
	sdma_dumpstate_helper(WFR_SEND_DMA_CHECK_VL);
	sdma_dumpstate_helper(WFR_SEND_DMA_CHECK_JOB_KEY);
	sdma_dumpstate_helper(WFR_SEND_DMA_CHECK_PARTITION_KEY);
	sdma_dumpstate_helper(WFR_SEND_DMA_CHECK_SLID);
	sdma_dumpstate_helper(WFR_SEND_DMA_CHECK_OPCODE);
}
#endif
