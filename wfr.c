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


static uint mod_num_sdma;
module_param_named(num_sdma, mod_num_sdma, uint, S_IRUGO);
MODULE_PARM_DESC(num_sdma, "Set max number SDMA engines to use");

uint kdeth_qp;
module_param_named(kdeth_qp, kdeth_qp, uint, S_IRUGO);
MODULE_PARM_DESC(kdeth_qp, "Set the KDETH queue pair prefix");

static uint num_vls = 4;
module_param(num_vls, uint, S_IRUGO);
MODULE_PARM_DESC(num_vls, "Set number of Virtual Lanes to use (1-8)");

static uint eager_buffer_size;
module_param(eager_buffer_size, uint, S_IRUGO);
MODULE_PARM_DESC(eager_buffer_size, "Size of the eager buffers, default max MTU`");

/* TODO: temporary */
static uint use_flr;
module_param_named(use_flr, use_flr, uint, S_IRUGO);
MODULE_PARM_DESC(use_flr, "Initialize the SPC with FLR");

/* TODO: temporary */
static uint print_unimplemented = 1;
module_param_named(print_unimplemented, print_unimplemented, uint, S_IRUGO);
MODULE_PARM_DESC(print_unimplemented, "Have unimplemented functions print when called");

/* TODO: temporary */
#define EASY_LINKUP_UNSET 100
static uint sim_easy_linkup = EASY_LINKUP_UNSET;

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

static u32 encoded_size(u32 size);
static u32 chip_to_ib_lstate(struct hfi_devdata *dd, u32 chip_lstate);
static int set_physical_link_state(struct hfi_devdata *dd, u64 state);

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

#if 0
u64 read_uctxt_csr(const struct hfi_devdata *dd, int ctxt, u32 offset0)
{
	/* user per-context CSRs are separated by 0x1000 */
	return read_csr(dd, offset0 + (0x1000 * ctxt));
}
#endif

static void write_uctxt_csr(struct hfi_devdata *dd, int ctxt, u32 offset0,
		u64 value)
{
	/* TODO: write to user mapping if available? */
	/* user per-context CSRs are separated by 0x1000 */
	write_csr(dd, offset0 + (0x1000 * ctxt), value);
} 

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
static char *is_sdma_err_name(char *buf, size_t bsize, unsigned int source)
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
	/* TODO: actually do something */
	printk("%s: engine #%d, status 0x%llx - unimplemented\n", __func__,
		per_sdma->which, status);
}

static char *pio_err_status_string(char *buf, int buf_len, u64 flags)
{
	return flag_string(buf, buf_len, flags,
			pio_err_status_flags, ARRAY_SIZE(pio_err_status_flags));
}

/*
 * CCE block "misc" interrupt.  Source is < 16.
 */
static void is_misc_err_int(struct hfi_devdata *dd, unsigned int source)
{
	char buf[96];
	u64 reg;

	switch (source) {
	case 4: /* PioErr */
		/* TODO: do more here.. most of these put the hfi in
		   freeze more.  We need to recognize that and unfreeze */
		/* clear the error(s) */
		reg = read_csr(dd, WFR_SEND_PIO_ERR_STATUS);
		write_csr(dd, WFR_SEND_PIO_ERR_CLEAR, reg);
		dd_dev_info(dd, "PIO Error: %s\n",
			pio_err_status_string(buf, sizeof(buf), reg));
		break;

	/* TODO: do something for the unhandled cases */
	case 0: /* CceErr */
	case 1: /* RxeErr */
	case 2: /* MiscErr */
	case 5: /* SDmaErr */
	case 6: /* EgressErr */
	case 7: /* TxeErr */
		printk("%s: int%u - unimplemented\n", __func__ , source);
		break;
	default: /* Reserved */
		dd_dev_err(dd, "Unexpected misc interrupt (%u) - reserved\n",
			source);
		break;
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
	u64 err_status;

	/* read and clear the error status */
	err_status = read_kctxt_csr(dd, source, WFR_SEND_CTXT_ERR_STATUS);
	if (err_status) {
		write_kctxt_csr(dd, source,
				WFR_SEND_CTXT_ERR_CLEAR, err_status);
		handle_send_context_err(dd, source, err_status);
	}
}

/*
 * CCE block SDMA error interrupt.  Source is < 16.
 */
static void is_sdma_err_int(struct hfi_devdata *dd, unsigned int source)
{
	/* TODO: actually do something */
	printk("%s: int%u - unimplemented\n", __func__ , source);
}

/*
 * CCE block "various" interrupt.  Source is < 12.
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

static void handle_8051_interrupt(struct hfi_devdata *dd, u64 reg)
{
	u64 info, err, host_msg;

	/* now look at the flags */
	if (reg & DC_DC8051_ERR_FLG_SET_BY_8051_SMASK) {
		/* 8051 information set by firmware */
		/* read DC8051_DBG_ERR_INFO_SET_BY_8051 for details */
		info = read_csr(dd, DC_DC8051_DBG_ERR_INFO_SET_BY_8051);
		err = (info >> DC_DC8051_DBG_ERR_INFO_SET_BY_8051_ERROR_SHIFT) & DC_DC8051_DBG_ERR_INFO_SET_BY_8051_ERROR_MASK;
		host_msg = (info >> DC_DC8051_DBG_ERR_INFO_SET_BY_8051_HOST_MSG_SHIFT) & DC_DC8051_DBG_ERR_INFO_SET_BY_8051_HOST_MSG_MASK;

		/*
		 * Handle error flags.
		 */
		if (err) {
			/* TODO: implement 8051 error handling */
			dd_dev_info(dd, "8051 info: error flags 0x%llx (unhandled)\n", err);
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
			/*
			 * TODO: check VerifyCap details
			 *
			 * These are now valid:
			 *	VerifyCap fields in the generic LNI config
			 *	CSR DC8051_STS_REMOTE_GUID
			 *	CSR DC8051_STS_REMOTE_NODE_TYPE
			 *	CSR DC8051_STS_REMOTE_FM_SECURITY
			 */
			/* tell the 8051 to go to LinkUp */
			set_physical_link_state(dd, WFR_PLS_LINKUP);
			/* clear flag so "uhnandled" message below
			   does not include this */
			host_msg &= ~(u64)WFR_VERIFY_CAP_FRAME;
		}
		/* look for unhandled flags */
		if (host_msg) {
			/* TODO: implement all other valid flags here */
			dd_dev_info(dd, "8051: host message flags 0x%llx (unhandled)\n", host_msg);
		}

		/* clear flag so "unhandled" message below does not
		   include this */
		reg &= ~DC_DC8051_ERR_FLG_SET_BY_8051_SMASK;
	}
	if (reg) {
		/* TODO: implement all other flags here */
		dd_dev_info(dd, "%s: 8051 Error: 0x%llx (unhandled)\n", __func__ , reg);
	}
}

/*
 * CCE block DC interrupt.  Source is < 8.
 */
static void is_dc_int(struct hfi_devdata *dd, unsigned int source)
{
	u64 reg;

	switch (source) {
	case 0: /* dc_common_int */
		/* read and clear the DCC error register */
		reg = read_csr(dd, DCC_ERR_FLG);
		if (reg)
			write_csr(dd, DCC_ERR_FLG_CLR, reg);
		/* TODO: implement DCC error handling */
		dd_dev_info(dd, "%s: DCC Error (dc%u): 0x%llx (unhandled)\n", __func__ , source, reg);
		break;
	case 1: /* dc_lcb_int */
		/* read and clear the LCB error register */
		reg = read_csr(dd, DC_LCB_ERR_FLG);
		if (reg)
			write_csr(dd, DC_LCB_ERR_CLR, reg);
		/* TODO: implement LCB error handling */
		dd_dev_info(dd, "%s: LCB Error (dc%u): 0x%llx (unhandled)\n", __func__ , source, reg);
		break;
	case 2: /* dc_8051_int */
		/* read and clear the 8051 error register */
		reg = read_csr(dd, DC_DC8051_ERR_FLG);
		if (reg) {
			write_csr(dd, DC_DC8051_ERR_CLR, reg);
			handle_8051_interrupt(dd, reg);
		}
		break;
	case 3: /* dc_lbm_int */
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
		break;
	default:
		dd_dev_err(dd, "Invalid DC interrupt %u\n", source);
		break;
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

	if (likely(what < 3 && which < dd->num_sdma)) {
		handle_sdma_interrupt(&dd->per_sdma[which], 1ull << source);
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
				is_sdma_err_name,	is_sdma_err_int },
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

	dd->int_counter++;

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

	dd->int_counter++;

	status = read_csr(dd,
			WFR_CCE_INT_STATUS + (8*(WFR_IS_SDMA_START/64)))
			& per_sdma->imask;
	if (likely(status)) {
		/* clear the interrupt(s) */
		write_csr(dd,
			WFR_CCE_INT_CLEAR + (8*(WFR_IS_SDMA_START/64)),
			per_sdma->imask);

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
	dd->int_counter++;

	/* clear the interrupt */
	write_csr(rcd->dd, WFR_CCE_INT_CLEAR + (8*rcd->ireg), rcd->imask);

	/* handle the interrupt */
	handle_receive_interrupt(rcd);

	return IRQ_HANDLED;
}

/* ========================================================================= */

static void sdma_sendctrl(struct qib_pportdata *ppd, unsigned op)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
}

static void sdma_hw_clean_up(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
}

static void sdma_update_tail(struct qib_pportdata *ppd, u16 tail)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
}

static void sdma_hw_start_up(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
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

	/*
	 * TODO: Do we want to hold the lock for this long?
	 * Alternatives:
	 * - keep busy wait - have other users bounce off
	 */
	spin_lock_irqsave(&dd->dc8051_lock, flags);

	/*
	 * NOTE: We expect that the command interface is in "neutral".
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
	timeout = jiffies + msecs_to_jiffies(5);
	while (1) {
		reg = read_csr(dd, DC_DC8051_CFG_HOST_CMD_1);
		completed = reg & DC_DC8051_CFG_HOST_CMD_1_COMPLETED_SMASK;
		if (completed)
			break;
		if (time_after(jiffies, timeout)) {
			dd_dev_err(dd, "host command timeout\n");
			if (out_data)
				*out_data = 0;
			return -ETIMEDOUT;
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

static int bringup_serdes(struct qib_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;
	u64 guid, reg;
	int ret;

	/* enable the port */
	/* TODO: 7322: done within rcvmod_lock */
	reg = read_csr(dd, WFR_RCV_CTRL);
	reg |= WFR_RCV_CTRL_RCV_PORT_ENABLE_SMASK;
	write_csr(dd, WFR_RCV_CTRL, reg);

	guid = be64_to_cpu(ppd->guid);
	if (!guid) {
		if (dd->base_guid)
			guid = be64_to_cpu(dd->base_guid) + ppd->port - 1;
		ppd->guid = cpu_to_be64(guid);
	}

	/* TODO: only start Polling if we have verified that media is
	   present */

	ret = set_physical_link_state(dd, WFR_PLS_POLLING);
	if (ret != WFR_HCMD_SUCCESS) {
		dd_dev_err(dd, "%s: set phsyical link state to Polling failed with return 0x%x\n", __func__, ret);

		if (ret >= 0)
			ret = -EINVAL;
		return ret;
	}

	return 0;
}

static void quiet_serdes(struct qib_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;
	u64 reg;
	int ret;

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
			     u32 type, unsigned long pa)
{
	u64 reg, bsize;

	if (type == PT_EAGER) {
		bsize = encoded_size(dd->rcvegrbufsize);
	} else if (type == PT_EXPECTED) {
		/* FIXME: expected (TID) sizes appear to be hard-coded to
		   PAGE_SIZE bytes */
		bsize = encoded_size(PAGE_SIZE);
	} else if (type == PT_INVALID) {
		bsize = 0;	/* invalid size, disables the entry */
		pa = 0;		/* remove former data */
	} else {
		dd_dev_err(dd,
			"unexpeced receive array type %u for index %u, not handled\n",
			type, index);
		return;
	}

	if (trace_tid)
		dd_dev_info(dd, "%s: type %s, index 0x%x, pa 0x%lx, bsize 0x%lx\n",
			__func__, pt_name(type), index, pa,
			(unsigned long)bsize);

#define RT_ADDR_SHIFT 12	/* 4KB kernel address boundary */
	reg = WFR_RCV_ARRAY_RT_WRITE_ENABLE_SMASK
		| bsize << WFR_RCV_ARRAY_RT_BUF_SIZE_SHIFT
		| ((pa >> RT_ADDR_SHIFT) & WFR_RCV_ARRAY_RT_ADDR_MASK)
					<< WFR_RCV_ARRAY_RT_ADDR_SHIFT;
	write_csr(dd, WFR_RCV_ARRAY + (index * 8), reg);
}

static void clear_tids(struct qib_ctxtdata *rcd)
{
	struct hfi_devdata *dd = rcd->dd;
	u32 i;

	/* TODO: this could be optimized */
	for (i = rcd->eager_base; i < rcd->eager_base + rcd->eager_count; i++)
		put_tid(dd, i, PT_INVALID, 0);

	for (i = rcd->expected_base;
			i < rcd->expected_base + rcd->expected_count; i++)
		put_tid(dd, i, PT_INVALID, 0);
}

static int get_base_info(struct qib_ctxtdata *rcd,
				  struct qib_base_info *kinfo)
{
	if (print_unimplemented)
		dd_dev_info(rcd->dd, "%s: not implemented\n", __func__);
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
 * @dd - device data
 * @mtu_bytes - mtu size, in bytes
 *
 * Set the MTU by limiting how many DWs may be sent.  The SendLenCheck*
 * registers compare against LRH.PktLen, so use the max bytes included
 * in the LRH.
 *
 * This routine changes all VL values except VL15, which it maintains at
 * the same value.
 */
static void set_send_length(struct hfi_devdata *dd, u32 mtu_bytes)
{
	u32 max_hb = lrh_max_header_bytes(dd);
	u64 len = ((mtu_bytes + max_hb) >> 2)
				& WFR_SEND_LEN_CHECK0_LEN_VL0_MASK;
	u64 vl15_len = ((MAX_MAD_PACKET  + max_hb) >> 2)
				& WFR_SEND_LEN_CHECK0_LEN_VL0_MASK;

	write_csr(dd, WFR_SEND_LEN_CHECK0,
		len << WFR_SEND_LEN_CHECK0_LEN_VL3_SHIFT
		| len << WFR_SEND_LEN_CHECK0_LEN_VL2_SHIFT
		| len << WFR_SEND_LEN_CHECK0_LEN_VL1_SHIFT
		| len << WFR_SEND_LEN_CHECK0_LEN_VL0_SHIFT);

	write_csr(dd, WFR_SEND_LEN_CHECK1,
		vl15_len << WFR_SEND_LEN_CHECK1_LEN_VL15_SHIFT
		| len << WFR_SEND_LEN_CHECK1_LEN_VL7_SHIFT
		| len << WFR_SEND_LEN_CHECK1_LEN_VL6_SHIFT
		| len << WFR_SEND_LEN_CHECK1_LEN_VL5_SHIFT
		| len << WFR_SEND_LEN_CHECK1_LEN_VL4_SHIFT);
}

static void set_lidlmc(struct qib_pportdata *ppd)
{
	u64 c1 = read_csr(ppd->dd, DCC_CFG_PORT_CONFIG1);
	c1 &= ~(DCC_CFG_PORT_CONFIG1_LID_SMASK|
	       DCC_CFG_PORT_CONFIG1_LMC_SMASK);
	c1 |= ((ppd->lid & DCC_CFG_PORT_CONFIG1_LID_MASK)
			<< DCC_CFG_PORT_CONFIG1_LID_SHIFT)|
	      ((ppd->lmc & DCC_CFG_PORT_CONFIG1_LMC_MASK)
			<< DCC_CFG_PORT_CONFIG1_LMC_SHIFT);
	write_csr(ppd->dd, DCC_CFG_PORT_CONFIG1, c1);
}

static int set_ib_cfg(struct qib_pportdata *ppd, int which, u32 val)
{
	struct hfi_devdata *dd = ppd->dd;
	u64 reg;
	u32 phys_request;
	int ret1, ret = 0;

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
		phys_request = val & 0xffff;
		switch (val & 0xffff0000) {
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
			dd_dev_info(dd,
			  "%s: which %s, val 0x%x: not implemented\n",
			  __func__, ib_cfg_name(which), val & 0xffff0000);
		}

		switch (phys_request) {
		case IB_LINKINITCMD_NOP:	/* nothing */
			break;
		case IB_LINKINITCMD_POLL:
			/* must transistion to offline first */
			ret1 = set_physical_link_state(dd, WFR_PLS_OFFLINE);
			if (ret1 != WFR_HCMD_SUCCESS) {
				dd_dev_err(dd, "Failed to transition to Offline link state, return 0x%x\n", ret1);
				ret = -EINVAL;
				break;
			}
			/*
			 * The above move to physical Offline state will 
			 * also move the logical state to Down.  Adjust
			 * the cached value.
			 * ** Do this before moving to Polling as the link
			 * may transition to LinkUp (and update the cache)
			 * before we do.
			 */
			ppd->lstate = IB_PORT_DOWN;
			ret1 = set_physical_link_state(dd, WFR_PLS_POLLING);
			if (ret1 != WFR_HCMD_SUCCESS) {
				dd_dev_err(dd, "Failed to transition to Polling link state, return 0x%x\n", ret1);
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
			/* must transistion to offline first */
			ret1 = set_physical_link_state(dd, WFR_PLS_OFFLINE);
			if (ret1 != WFR_HCMD_SUCCESS) {
				dd_dev_err(dd, "Failed to transition to Offline link state, return 0x%x\n", ret1);
				ret = -EINVAL;
				break;
			}
			/*
			 * The above move to physical Offline state will 
			 * also move the logical state to Down.  Adjust
			 * the cached value.
			 */
			ppd->lstate = IB_PORT_DOWN;
			ret1 = set_physical_link_state(dd, WFR_PLS_DISABLED);
			if (ret1 != WFR_HCMD_SUCCESS) {
				dd_dev_err(dd, "Failed to transition to Disabled link state, return 0x%x\n", ret1);
				ret = -EINVAL;
			}
			break;
		default:
			dd_dev_info(dd,
			  "%s: which %s, val 0x%x: not implemented\n",
			  __func__, ib_cfg_name(which), val & 0xffff);
		}
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
		set_send_length(dd, val);
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

static int get_ib_table(struct qib_pportdata *ppd, int which, void *t)
{
	switch (which) {
	case QIB_IB_TBL_VL_HIGH_ARB:
		get_vl_weights(ppd->dd, WFR_SEND_HIGH_PRIORITY_LIST,
			WFR_VL_ARB_HIGH_PRIO_TABLE_SIZE, t);
		break;
	case QIB_IB_TBL_VL_LOW_ARB:
		get_vl_weights(ppd->dd, WFR_SEND_LOW_PRIORITY_LIST,
			WFR_VL_ARB_LOW_PRIO_TABLE_SIZE, t);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int set_ib_table(struct qib_pportdata *ppd, int which, void *t)
{
	switch (which) {
	case QIB_IB_TBL_VL_HIGH_ARB:
		set_vl_weights(ppd->dd, WFR_SEND_HIGH_PRIORITY_LIST,
			WFR_VL_ARB_HIGH_PRIO_TABLE_SIZE, t);
		break;
	case QIB_IB_TBL_VL_LOW_ARB:
		set_vl_weights(ppd->dd, WFR_SEND_LOW_PRIORITY_LIST,
			WFR_VL_ARB_LOW_PRIO_TABLE_SIZE, t);
		break;
	default:
		return -EINVAL;
	}
	return 0;
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
	// FIXME: We're hard-coding the counter to 1 here.  We need
	// a scheme - likely integrated with a timeout.
	reg = (1ull << WFR_RCV_HDR_HEAD_COUNTER_SHIFT) |
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
 *	0x8-0xF - reserved (Context Control)
 *	0xB-0xF - reserved (Receive Array)
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
	u32 eager_header_counter = 0;	/* non-zero means do something */
	int hp = 0;			/* header printed? */

	rcd = dd->rcd[ctxt];
	if (!rcd)
		return;

	// FIXME: QIB_RCVCTRL_PKEY_ENB/DIS
	if (op & QIB_RCVCTRL_PKEY_ENB)
		rcvctrl_unimplemented(dd, __func__, ctxt, op, &hp, "RCVCTRL_PKEY_ENB");
	if (op & QIB_RCVCTRL_PKEY_DIS)
		rcvctrl_unimplemented(dd, __func__, ctxt, op, &hp, "RCVCTRL_PKEY_DIS");
	// FIXME: QIB_RCVCTRL_BP_ENB/DIS
	if (op & QIB_RCVCTRL_BP_ENB)
		rcvctrl_unimplemented(dd, __func__, ctxt, op, &hp, "RCVCTRL_BP_ENB");
	if (op & QIB_RCVCTRL_BP_DIS)
		rcvctrl_unimplemented(dd, __func__, ctxt, op, &hp, "RCVCTRL_BP_DIS");

	rcvctrl = read_kctxt_csr(dd, ctxt, WFR_RCV_CTXT_CTRL);
	/* if the context already enabled, don't do the extra steps */
	if ((op & QIB_RCVCTRL_CTXT_ENB)
			&& !(rcvctrl & WFR_RCV_CTXT_CTRL_ENABLE_SMASK)) {
		/* reset the tail and hdr addresses, and sequence count */
		write_kctxt_csr(dd, ctxt, WFR_RCV_HDR_TAIL_ADDR, rcd->rcvhdrqtailaddr_phys);
		write_kctxt_csr(dd, ctxt, WFR_RCV_HDR_ADDR, rcd->rcvhdrq_phys);

		rcd->seq_cnt = 1;
		/*
		 * When the context enable goes from 0 to 1, RcvEgrIndexHead,
		 * RcvEgrIndexTail, and RcvEgrOffsetTail are reset to 0.
		 * However, once we activate the context, we need to add a
		 * count of (at least) 1 to RcvEgrHdrHead.Counter so
		 * interrupts are generated.
		 * TODO: We don't always want interrupts.  Need a
		 * mechanism for that. Also, we could make this
		 * conditional on IntrAvail.  Not sure it we need to
		 * worry about it, though.
		 */
		rcvctrl |= WFR_RCV_CTXT_CTRL_ENABLE_SMASK;
		eager_header_counter = 1; /* non-zero means do something */

		/* set the control's eager buffer size */
		rcvctrl &= ~WFR_RCV_CTXT_CTRL_EGR_BUF_SIZE_SMASK;
		rcvctrl |= (encoded_size(dd->rcvegrbufsize)
				& WFR_RCV_CTXT_CTRL_EGR_BUF_SIZE_MASK)
					<< WFR_RCV_CTXT_CTRL_EGR_BUF_SIZE_SHIFT;

		/* FIXME: for now, always set OnePacketPerEgrBuffer until
		   we know the driver can handle packed eager buffers */
		rcvctrl |= WFR_RCV_CTXT_CTRL_ONE_PACKET_PER_EGR_BUFFER_SMASK;

		/* set eager count and base index */
		reg = ((rcd->eager_count & WFR_RCV_EGR_CTRL_EGR_CNT_MASK)
				<< WFR_RCV_EGR_CTRL_EGR_CNT_SHIFT) |
		      ((rcd->eager_base & WFR_RCV_EGR_CTRL_EGR_BASE_INDEX_MASK)
				<< WFR_RCV_EGR_CTRL_EGR_BASE_INDEX_SHIFT);
		write_kctxt_csr(dd, ctxt, WFR_RCV_EGR_CTRL, reg);

		/* set TID (expected) count and base index */
		reg = ((rcd->expected_count &
					WFR_RCV_TID_CTRL_TID_PAIR_CNT_MASK)
				<< WFR_RCV_TID_CTRL_TID_PAIR_CNT_SHIFT) |
		      ((rcd->expected_base &
					WFR_RCV_TID_CTRL_TID_BASE_INDEX_MASK)
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
	write_kctxt_csr(dd, ctxt, WFR_RCV_CTXT_CTRL, rcvctrl);

	/*
	 * This must be done after we transition a context to enabled
	 * in RcvCtxtCtrl.
	 */
	if (eager_header_counter) {
		/*
		 * ASSUMPTION: we've just transitioned the context
		 * to enabled with the write to RcvCtxtCtrl above.
		 * This will zero out RcvHdrHead, so we only need to
		 * update the Counter field.
		 */
		reg = (eager_header_counter & WFR_RCV_HDR_HEAD_COUNTER_MASK)
			<< WFR_RCV_HDR_HEAD_COUNTER_SHIFT;
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
	if (print_unimplemented)
		dd_dev_info(dd, "%s: not implemented\n", __func__);
	return 0; /* final read after getting everything */
}

static u32 read_portcntrs(struct hfi_devdata *dd, loff_t pos, u32 port,
				  char **namep, u64 **cntrp)
{
	static int called;
	if (!called) {
		called = 1;
		if (print_unimplemented)
			dd_dev_info(dd, "%s: not implemented\n", __func__);
	}
	return 0; /* final read after getting everything */
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
	return ppd->lstate;
}

static u8 ibphys_portstate(struct qib_pportdata *ppd)
{
	static u32 remembered_state = 0xff;
	u32 ib_pstate;

	ib_pstate = chip_to_ib_pstate(ppd->dd, read_physical_state(ppd->dd));
	if (remembered_state != ib_pstate) {
		dd_dev_info(ppd->dd,
			"%s: physical state changed to %s (0x%x)\n", __func__,
			ib_pstate_name(ib_pstate), ib_pstate);
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
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
}

static void sdma_init_early(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
}

static int init_sdma_regs(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
	return 0;
}

static u16 sdma_gethead(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
	return 0;
}

static int sdma_busy(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
	return 0;
}

/*
 * QIB sets these rcd fields in this function:
 *	rcvegrcnt	 (now eager_count)
 *	rcvegr_tid_base  (now eager_base)
 */
static void init_ctxt(struct qib_ctxtdata *rcd)
{
	struct hfi_devdata *dd = rcd->dd;
	u64 reg;
	u32 context = rcd->ctxt;

	/*
	 * Simple allocation: we have already pre-allocated the number
	 * of entries per context in dd->rcv_entries.  Now, divide the
	 * per context value by 2 and use half for eager and expected.
	 * This requires that the count be divisible by 4 as the expected
	 * array base and count must be divisible by 2.
	 */
	rcd->eager_count = dd->rcv_entries / 2;
	if (rcd->eager_count > WFR_MAX_EAGER_ENTRIES)
		rcd->eager_count = WFR_MAX_EAGER_ENTRIES;
	rcd->expected_count = dd->rcv_entries / 2;
	if (rcd->expected_count > WFR_MAX_TID_PAIR_ENTRIES * 2)
		rcd->expected_count = WFR_MAX_TID_PAIR_ENTRIES * 2;
	rcd->eager_base = (dd->rcv_entries * context);
	rcd->expected_base = rcd->eager_base + rcd->eager_count;
	BUG_ON(rcd->expected_base % 2 == 1);	/* must be even */
	BUG_ON(rcd->expected_count % 2 == 1);	/* must be even */

	/*
	 * These values are per-context:
	 *	RcvHdrCnt
	 *	RcvHdrEntSize
	 *	RcvHdrSize
	 * For now, all contexts get the same values from dd.
	 * TODO: optimize these on a per-context basis.
	 */
	reg = (dd->rcvhdrcnt & WFR_RCV_HDR_CNT_CNT_MASK)
		<< WFR_RCV_HDR_CNT_CNT_SHIFT;
	write_kctxt_csr(dd, context, WFR_RCV_HDR_CNT, reg);
	reg = (dd->rcvhdrentsize & WFR_RCV_HDR_ENT_SIZE_ENT_SIZE_MASK)
		<< WFR_RCV_HDR_ENT_SIZE_ENT_SIZE_SHIFT;
	write_kctxt_csr(dd, context, WFR_RCV_HDR_ENT_SIZE, reg);
	reg = (dd->rcvhdrsize & WFR_RCV_HDR_SIZE_HDR_SIZE_MASK)
		<< WFR_RCV_HDR_SIZE_HDR_SIZE_SHIFT;
	write_kctxt_csr(dd, context, WFR_RCV_HDR_SIZE, reg);
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
		write_csr(dd, DCC_ERR_FLG_EN, ~(u64)0);
		write_csr(dd, DC_LCB_ERR_EN, ~(u64)0);
		write_csr(dd, DC_DC8051_ERR_EN, ~(u64)0);
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
		write_csr(dd, DCC_ERR_FLG_EN, 0);
		write_csr(dd, DC_LCB_ERR_EN, 0);
		write_csr(dd, DC_DC8051_ERR_EN, 0);
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
	last_rx = first_rx + dd->num_rcv_contexts;

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
			/* no interrupt for user contexts */
			if (idx >= dd->first_user_ctxt)
				continue;
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
	 *	M interrupt - one per used rx context
	 *	TODO: pio (tx) contexts?
	 */
	total = 1 + dd->num_sdma + dd->num_rcv_contexts;

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
	/* TODO: clear diagnostic reg? */

	/* reset general handler mask, chip MSI-X mappings */
	reset_interrupts(dd);

	/*
	 * Intialize per-SDMA data, so we have a unique pointer to hand to
	 * specialized handlers.
	 */
	for (i = 0; i < dd->num_sdma; i++) {
		dd->per_sdma[i].dd = dd;
		dd->per_sdma[i].which = i;
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
	dd->int_counter++; /* fake an interrupt so we stop getting called */
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
	u32 per_context;

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
	 * Simple recieve array allocation:  Evenly divide them
	 * Requirements:
	 *	- Expected TID indices must be even (they are pairs)
	 *
	 * Alogrithm.  Make the requirement always true by:
	 *	- making the per-context count be divisible by 4
	 *	- evenly divide between eager and TID count
	 *
	 * TODO: Make this more sophisticated
	 */
	per_context = dd->chip_rcv_array_count / dd->num_rcv_contexts;
	per_context -= (per_context % 4);
	/* FIXME: no more than 24 each of eager and expected TIDs, for now! */
#define TEMP_MAX_ENTRIES 48
	if (per_context > TEMP_MAX_ENTRIES)
		per_context = TEMP_MAX_ENTRIES;
	dd->rcv_entries = per_context;
	dd_dev_info(dd, "rcv entries %u\n", dd->rcv_entries);

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
 * The partition key values are undefined after reset.
 * Set up the minimal partition keys:
 *	- 0xffff in the first key
	- 0 in all other keys
 */
static void init_partition_keys(struct hfi_devdata *dd)
{
	write_csr(dd, WFR_RCV_PARTITION_KEY + (0 * 8), 
		(DEFAULT_PKEY & WFR_RCV_PARTITION_KEY_PARTITION_KEY_A_MASK)
			<< WFR_RCV_PARTITION_KEY_PARTITION_KEY_A_SHIFT);
	write_csr(dd, WFR_RCV_PARTITION_KEY + (1 * 8),  0);
	write_csr(dd, WFR_RCV_PARTITION_KEY + (2 * 8),  0);
	write_csr(dd, WFR_RCV_PARTITION_KEY + (3 * 8),  0);
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
}

/* set TXE CSRs to chip reset defaults */
static void reset_txe(struct hfi_devdata *dd)
{
	int i;

	write_csr(dd, WFR_SEND_CTRL, 0);
	write_csr(dd, WFR_SEND_HIGH_PRIORITY_LIMIT, 0);
	/* TODO: wait for SendPioInitCtxt.PioInitInProgress to be clear? */
	write_csr(dd, WFR_SEND_PIO_ERR_MASK, 0);
	write_csr(dd, WFR_SEND_PIO_ERR_CLEAR, ~0ull);
	write_csr(dd, WFR_SEND_DMA_ERR_MASK, 0);
	write_csr(dd, WFR_SEND_DMA_ERR_CLEAR, ~0ull);
	write_csr(dd, WFR_SEND_EGRESS_ERR_MASK, 0);
	write_csr(dd, WFR_SEND_EGRESS_ERR_CLEAR, ~0ull);
	write_csr(dd, WFR_SEND_BTH_QP, 0);
	write_csr(dd, WFR_SEND_STATIC_RATE_CONTROL, 0);
	write_csr(dd, WFR_SEND_SC2VLT0, 0);
	write_csr(dd, WFR_SEND_SC2VLT1, 0);
	write_csr(dd, WFR_SEND_SC2VLT2, 0);
	write_csr(dd, WFR_SEND_SC2VLT3, 0);
	write_csr(dd, WFR_SEND_LEN_CHECK0, 0);
	write_csr(dd, WFR_SEND_LEN_CHECK1, 0);
	write_csr(dd, WFR_SEND_ERR_MASK, 0);
	write_csr(dd, WFR_SEND_ERR_CLEAR, ~0ull);
	for (i = 0; i < WFR_VL_ARB_LOW_PRIO_TABLE_SIZE; i++)
		write_csr(dd, WFR_SEND_LOW_PRIORITY_LIST + (8*i), 0);
	for (i = 0; i < WFR_VL_ARB_HIGH_PRIO_TABLE_SIZE; i++)
		write_csr(dd, WFR_SEND_HIGH_PRIORITY_LIST + (8*i), 0);
	for (i = 0; i < WFR_TXE_NUM_CONTEXT_SET; i++)
		write_csr(dd, WFR_SEND_CONTEXT_SET_CTRL + (8*i), 0);
	for (i = 0; i < WFR_TXE_NUM_32_BIT_COUNTER; i++)
		write_csr(dd, WFR_SEND_COUNTER_ARRAY32 + (8*i), 0);
	for (i = 0; i < WFR_TXE_NUM_64_BIT_COUNTER; i++)
		write_csr(dd, WFR_SEND_COUNTER_ARRAY64 + (8*i), 0);
	write_csr(dd, WFR_SEND_CM_CTRL, 0x2 << 4);
	write_csr(dd, WFR_SEND_CM_GLOBAL_CREDIT, 0);
	write_csr(dd, WFR_SEND_CM_TIMER_CTRL, 0);
	write_csr(dd, WFR_SEND_CM_LOCAL_AU_TABLE0_TO3, 0);
	write_csr(dd, WFR_SEND_CM_LOCAL_AU_TABLE4_TO7, 0);
	write_csr(dd, WFR_SEND_CM_REMOTE_AU_TABLE0_TO3, 0);
	write_csr(dd, WFR_SEND_CM_REMOTE_AU_TABLE4_TO7, 0);
	for (i = 0; i < WFR_TXE_NUM_DATA_VL; i++)
		write_csr(dd, WFR_SEND_CM_CREDIT_VL + (8*i), 0);
	write_csr(dd, WFR_SEND_CM_CREDIT_VL15, 0);
	write_csr(dd, WFR_SEND_EGRESS_ERR_INFO, ~0ull);

	for (i = 0; i < dd->chip_send_contexts; i++) {
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CTRL, 0);
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CREDIT_CTRL, 0);
		/* WFR_SEND_CTXT_CREDIT_RETURN_ADDR initialized elsewhere */
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

	//FIXME: Add SDMA registers
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
 */
static void init_chip(struct hfi_devdata *dd)
{
	u64 reg, mask;
	int i;

	dd->chip_rcv_contexts = read_csr(dd, WFR_RCV_CONTEXTS);
	dd->chip_rcv_array_count = read_csr(dd, WFR_RCV_ARRAY_CNT);
	dd->chip_send_contexts = read_csr(dd, WFR_SEND_CONTEXTS);
	dd->chip_sdma_engines = read_csr(dd, WFR_SEND_DMA_ENGINES);
	dd->chip_pio_mem_size = read_csr(dd, WFR_SEND_PIO_MEM_SIZE);

	/*
	 * If we are holding the ASIC mutex, clear it.
	 */
	reg = read_csr(dd, WFR_ASIC_CFG_MUTEX);
	mask = 1ull << dd->hfi_id;
	if (reg & mask)
		write_csr(dd, WFR_ASIC_CFG_MUTEX, 0);

	if (use_flr) {
		dd_dev_info(dd, "Using FLR+DC reset to clear CSRs\n");
		/*
		 * Reset the device to put the CSRs in a known state.
		 * A FLR will reset the SPC core without resetting the
		 * PCIe.  Combine this with a DC reset.
		 *
		 * Stop the device from doing anything while we do a
		 * reset.  We know there are no other active users of
		 * the device since we are now in charge.  Next, turn
		 * off all outbound and inbound traffic and make sure
		 * the device does not generate any interrupts.
		 */

		/* disable send contexts and SDMA engines */
		for (i = 0; i < dd->chip_send_contexts; i++)
			write_kctxt_csr(dd, i, WFR_SEND_CTXT_CTRL, 0);
		for (i = 0; i < dd->chip_sdma_engines; i++)
			write_kctxt_csr(dd, i, WFR_SEND_DMA_CTRL, 0);
		/* disable port (turn off RXE inbound traffic) */
		write_csr(dd, WFR_RCV_CTRL, 0);
		/* mask all interrupt sources */
		for (i = 0; i < WFR_CCE_NUM_INT_CSRS; i++)
			write_csr(dd, WFR_CCE_INT_MASK + (8*i), 0ull);

		/*
		 * DC Reset: do a full DC reset before the FLR.  A
		 * recommended length of time to hold is one CSR read,
		 * so reread the CceDcCtrl.  Then, hold the DC in reset
		 * across the FLR.
		 */
		write_csr(dd, WFR_CCE_DC_CTRL, WFR_CCE_DC_CTRL_DC_RESET_SMASK);
		(void) read_csr(dd, WFR_CCE_DC_CTRL);

		/* do the FLR, the DC reset will remain */
		hfi_pcie_flr(dd);

		/* now clear the DC reset */
		write_csr(dd, WFR_CCE_DC_CTRL, 0);
	} else {
		dd_dev_info(dd, "Clearing CSRs with writes\n");
		// FIXME:
		// o make sure all rcv contexts are disabled
		// o make sure all of SDMA engines are disabled
		// o make sure all interrupts are disabled
		// o reset all interrupt mappings back to default
		// o other...
		reset_txe(dd);
	}

	write_uninitialized_csrs_and_memories(dd);

	init_partition_keys(dd);

	/*
	 * TODO: The following block is strictly for the simulator.
	 * If we find that the physical and logical states are LinkUp
	 * and Active respectively, then assume we are in the simulator's
	 * "EasyLinkup" mode.  When in EayLinkup mode, we don't change
	 * the DC's physical and logical states.
	 */
	{
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

void init_rxe(struct hfi_devdata *dd)
{
	/* enable all receive errors */
	write_csr(dd, WFR_RCV_ERR_MASK, ~0ull);

	/* TODO: others...? */
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
/* TODO: In HAS 0.76 this changes to 1 AU, not 1 CU */
/* NOTE: The has may not matter so much as the sim version */
#if 0
		|  1ull <<
			WFR_SEND_CM_LOCAL_AU_TABLE0_TO3_LOCAL_AU_TABLE1_SHIFT
#else
		|  1ull * cu <<
			WFR_SEND_CM_LOCAL_AU_TABLE0_TO3_LOCAL_AU_TABLE1_SHIFT
#endif
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

static void assign_local_cm_au_table(struct hfi_devdata *dd, u32 cu)
{
	assign_cm_au_table(dd, cu, WFR_SEND_CM_LOCAL_AU_TABLE0_TO3,
					WFR_SEND_CM_LOCAL_AU_TABLE4_TO7);
}

static void assign_remote_cm_au_table(struct hfi_devdata *dd, u32 cu)
{
	assign_cm_au_table(dd, cu, WFR_SEND_CM_REMOTE_AU_TABLE0_TO3,
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

	/* set the local CU to AU mapping */
	assign_local_cm_au_table(dd, hfi_cu);

	/*
	 * FIXME: Set up initial send link credits.  We need an amount
	 * in vl15 so we can talk to the FM to set up the rest.  The
	 * value used here is completely arbitrary.  Find the right
	 * value.  IN ADDITION: set up credits on the other VLs as
	 * we do not have a FM to talk to in the current simulation
	 * environment.  These values are also completely arbitrary.
	 * Plus there is no explanation for the fields.  Should the
	 * global limit be the sum of the other limits?
	 */
#define WFR_CM_AU ((u64)3) /* allocation unit - 64b */
			   /*   this is the only valid value for WFR, PRR */
#define WFR_CM_GLOBAL_SHARED_LIMIT	((u64)0x1000) /* arbitrary */
#define WFR_CM_VL_DEDICATED_CREDITS	((u64)0x0100) /* arbitrary */
#define WFR_CM_VL_SHARED_CREDITS	(WFR_CM_GLOBAL_SHARED_LIMIT/2)
	write_csr(dd, WFR_SEND_CM_GLOBAL_CREDIT,
			(WFR_CM_GLOBAL_SHARED_LIMIT
			    << WFR_SEND_CM_GLOBAL_CREDIT_GLOBAL_LIMIT_SHIFT)
			| (WFR_CM_AU
			    << WFR_SEND_CM_GLOBAL_CREDIT_AU_SHIFT));
	for (i = 0; i < WFR_TXE_NUM_DATA_VL; i++) {
		write_csr(dd, WFR_SEND_CM_CREDIT_VL + (8 * i),
			(WFR_CM_VL_DEDICATED_CREDITS
			    << WFR_SEND_CM_CREDIT_VL_DEDICATED_LIMIT_VL_SHIFT)
			| (WFR_CM_VL_SHARED_CREDITS
			    << WFR_SEND_CM_CREDIT_VL_SHARED_LIMIT_VL_SHIFT));
	}
	write_csr(dd, WFR_SEND_CM_CREDIT_VL15,
			(WFR_CM_VL_DEDICATED_CREDITS
			    << WFR_SEND_CM_CREDIT_VL15_DEDICATED_LIMIT_VL_SHIFT)
			| (WFR_CM_VL_SHARED_CREDITS
			    << WFR_SEND_CM_CREDIT_VL15_SHARED_LIMIT_VL_SHIFT));
	assign_remote_cm_au_table(dd, 1);	/* 1 CU = 1 AU */
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
	u16 revision;
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
	dd->num_pports = NUM_IB_PORTS;

	/* pport structs are contiguous, allocated after devdata */
	ppd = (struct qib_pportdata *)(dd + 1);
	dd->pport = ppd;
	for (i = 0; i < dd->num_pports; i++) {
		/* init common fields */
		qib_init_pportdata(&ppd[i], dd, 0, 1);
		/* chip specific */
		/* TODO: correct for STL */
		ppd[i].link_speed_supported = IB_SPEED_EDR;
		ppd[i].link_width_supported = IB_WIDTH_1X | IB_WIDTH_4X;
		ppd[i].link_width_enabled = IB_WIDTH_4X;
		ppd[i].link_speed_enabled = ppd[i].link_speed_supported;

		switch (num_vls) {
		case 1:
			ppd[i].vls_supported = IB_VL_VL0;
			break;
		case 2:
			ppd[i].vls_supported = IB_VL_VL0_1;
			break;
		default:
			dd_dev_info(dd,
				    "Invalid num_vls %u, using 4 VLs\n",
				    num_vls);
			num_vls = 4;
			/* fall through */
		case 4:
			ppd[i].vls_supported = IB_VL_VL0_3;
			break;
		case 8:
			ppd[i].vls_supported = IB_VL_VL0_7;
			break;
		}
		ppd[i].vls_operational = ppd[i].vls_supported;

		/*
		 * Set the initial values to reasonable default, will be set
		 * for real when link is up.
		 */
		ppd[i].link_width_active = IB_WIDTH_4X;
		ppd[i].link_speed_active = IB_SPEED_EDR;
		ppd[i].lstate = IB_PORT_DOWN;
		ppd[i].overrun_threshold = 0x4;
		ppd[i].phy_error_threshold = 0xf;
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
	dd->f_get_ib_table      = get_ib_table;
	dd->f_set_ib_table      = set_ib_table;
	dd->f_set_intr_state    = set_intr_state;
	dd->f_setextled         = setextled;
	dd->f_update_usrhead    = update_usrhead;
	dd->f_wantpiobuf_intr   = sc_wantpiobuf_intr;
	dd->f_xgxs_reset        = xgxs_reset;
	dd->f_sdma_hw_clean_up  = sdma_hw_clean_up;
	dd->f_sdma_hw_start_up  = sdma_hw_start_up;
	dd->f_sdma_init_early   = sdma_init_early;
	dd->f_tempsense_rd	= tempsense_rd;

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
		goto bail_free;
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
	revision = rev2 >> WFR_CCE_REVISION2_IMPL_REVISION_SHIFT;
	dd_dev_info(dd, "Implementation: %s, revision 0x%x\n",
		dd->icode < ARRAY_SIZE(inames) ? inames[dd->icode] : "unknown",
		(int)revision);

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
	dd->num_sdma = dd->chip_sdma_engines;
	if (mod_num_sdma && mod_num_sdma < dd->chip_sdma_engines)
		dd->num_sdma = mod_num_sdma;

	/* FIXME: don't set NODMA_RTAIL until we know the sequence numbers
	   are right */
	//dd->flags = QIB_NODMA_RTAIL;

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

	/* set up the stats timer; the add_timer is done at end of init */
	init_timer(&dd->stats_timer);
	dd->stats_timer.function = get_faststats;
	dd->stats_timer.data = (unsigned long) dd;

	/* TODO: RcvHdrEntSize, RcvHdrCnt, and RcvHdrSize are now
	   per context, rather than global. */
	/* FIXME: arbitrary/old values */
	dd->rcvhdrcnt = 64;
#if 1
	dd->rcvhdrentsize = DEFAULT_RCVHDR_ENTSIZE;
	dd->rcvhdrsize = DEFAULT_RCVHDRSIZE;
#else
	dd->rcvhdrentsize = qib_rcvhdrentsize ?
		qib_rcvhdrentsize : DEFAULT_RCVHDR_ENTSIZE;
	dd->rcvhdrsize = qib_rcvhdrsize ?
		qib_rcvhdrsize : DEFAULT_RCVHDRSIZE;
#endif
	dd->rhf_offset = dd->rcvhdrentsize - sizeof(u64) / sizeof(u32);

	ret = set_up_context_variables(dd);
	if (ret)
		goto bail_cleanup;

	/* set initial RXE CSRs */
	init_rxe(dd);
	/* set initial TXE CSRs */
	init_txe(dd);
	/* set up KDETH QP prefix in both RX and TX CSRs */
	init_kdeth_qp(dd);

	/* send contexts must be set up before receive contexts */
	ret = init_send_contexts(dd);
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

	/*
	 * TODO: RX init, TX init
	 * 
	 * Set the CSRs to sane, expected values - the driver can be
	 * loaded and unloaded, we can't expect reset values.
	 */

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

/* interrupt testing */
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
}
