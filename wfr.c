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

/* TODO: temporary */
static uint print_unimplemented = 1;
module_param_named(print_unimplemented, print_unimplemented, uint, S_IRUGO);
MODULE_PARM_DESC(print_unimplemented, "Have unimplemented functions print when called");

uint default_link_state = IB_PORT_ACTIVE;
module_param(default_link_state, uint, S_IRUGO);
MODULE_PARM_DESC(default_link_state, "Set to IB_PORT_INIT to allow external setting of lid, state");

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
	return read_csr(dd, offset0 + (0x100* ctxt));
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
 * CCE block DC interrupt.  Source is < 8.
 */
static void is_dc_int(struct hfi_devdata *dd, unsigned int source)
{
	/* TODO: actually do something */
	printk("%s: int%u - unimplemented\n", __func__ , source);
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
		/* TODO: the print is temporary */
		char buf[64];
		printk(DRIVER_NAME"%d: interrupt %d: %s\n", dd->unit, bit,
			is_name(buf, sizeof(buf), bit));
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

static int bringup_serdes(struct qib_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;
	u64 guid, reg;

	if (print_unimplemented)
		dd_dev_info(dd, "%s: ppd 0x%p: not implemented (enabling port)\n", __func__, ppd);

	/* enable the port */
	/* NOTE: 7322: done within rcvmod_lock */
	reg = read_csr(dd, WFR_RCV_CTRL);
	reg |= WFR_RCV_CTRL_RCV_PORT_ENABLE_SMASK;
	write_csr(dd, WFR_RCV_CTRL, reg);
	/* TODO: clear RcvCtrl.RcvPortEnable in quiet_serdes()? */

	/* 7322: enable the serdes status change interrupt */

	guid = be64_to_cpu(ppd->guid);
	if (!guid) {
		if (dd->base_guid)
			guid = be64_to_cpu(dd->base_guid) + ppd->port - 1;
		ppd->guid = cpu_to_be64(guid);
	}

	/* stub lstate stuff */
	ppd->lstate = default_link_state;
	switch (default_link_state) {
	case IB_PORT_INIT:
		ppd->lflags |= (QIBL_LINKV|QIBL_LINKINIT);
		break;
	case IB_PORT_ACTIVE:
		ppd->lflags |= (QIBL_LINKV|QIBL_LINKACTIVE);
		break;
	}
	return 0;
}

static void quiet_serdes(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
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

static void wantpiobuf_intr(struct hfi_devdata *dd, u32 needint)
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

	dd_dev_info(dd, "%s: type %s, index 0x%x, pa 0x%lx, bsize 0x%lx\n",
		__func__, pt_name(type), index, pa, (unsigned long)bsize);

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
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: which %s: not implemented\n", __func__, ib_cfg_name(which));
	return 0;
}

static int set_ib_cfg(struct qib_pportdata *ppd, int which, u32 val)
{
	switch (which) {
	case QIB_IB_CFG_LSTATE:
		switch (val & 0xffff0000) {
		case IB_LINKCMD_ARMED:
			ppd->lflags |= (QIBL_LINKV|QIBL_LINKARMED);
			ppd->lstate = IB_PORT_ARMED;
			break;
		case IB_LINKCMD_ACTIVE:
			ppd->lflags |= (QIBL_LINKV|QIBL_LINKACTIVE);
			ppd->lstate = IB_PORT_ACTIVE;
			break;
		}
		break;
	default:
		if (print_unimplemented)
			dd_dev_info(ppd->dd,
			  "%s: which %s, val 0x%x: not implemented\n",
			  __func__, ib_cfg_name(which), val);
		break;
	}
	return 0;
}

static int set_ib_loopback(struct qib_pportdata *ppd, const char *what)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
	return 0;
}

static int get_ib_table(struct qib_pportdata *ppd, int which, void *t)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
	return 0;
}

static int set_ib_table(struct qib_pportdata *ppd, int which, void *t)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
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

	rcd = dd->rcd[ctxt];
	if (!rcd)
		return;

	dd_dev_info(dd, "%s: context %d, flags:\n", __func__, ctxt);
	if (op & QIB_RCVCTRL_TAILUPD_ENB)
		dd_dev_info(dd, "    QIB_RCVCTRL_TAILUPD_ENB\n");
	if (op & QIB_RCVCTRL_TAILUPD_DIS)
		dd_dev_info(dd, "    QIB_RCVCTRL_TAILUPD_DIS\n");
	if (op & QIB_RCVCTRL_CTXT_ENB)
		dd_dev_info(dd, "    QIB_RCVCTRL_CTXT_ENB\n");
	if (op & QIB_RCVCTRL_CTXT_DIS)
		dd_dev_info(dd, "    QIB_RCVCTRL_CTXT_DIS\n");
	if (op & QIB_RCVCTRL_INTRAVAIL_ENB)
		dd_dev_info(dd, "    QIB_RCVCTRL_INTRAVAIL_ENB\n");
	if (op & QIB_RCVCTRL_INTRAVAIL_DIS)
		dd_dev_info(dd, "    QIB_RCVCTRL_INTRAVAIL_DIS\n");
	// FIXME: QIB_RCVCTRL_PKEY_ENB/DIS
	if (op & QIB_RCVCTRL_PKEY_ENB)
		dd_dev_info(dd, "    QIB_RCVCTRL_PKEY_ENB ** NOT IMPLEMENTED **\n");
	if (op & QIB_RCVCTRL_PKEY_DIS)
		dd_dev_info(dd, "    QIB_RCVCTRL_PKEY_DIS ** NOT IMPLEMENTED **\n");
	// FIXME: QIB_RCVCTRL_BP_ENB/DIS
	if (op & QIB_RCVCTRL_BP_ENB)
		dd_dev_info(dd, "    QIB_RCVCTRL_BP_ENB ** NOT IMPLEMENTED **\n");
	if (op & QIB_RCVCTRL_BP_DIS)
		dd_dev_info(dd, "    QIB_RCVCTRL_BP_DIS ** NOT IMPLEMENTED **\n");
	if (op & QIB_RCVCTRL_TIDFLOW_ENB)
		dd_dev_info(dd, "    QIB_RCVCTRL_TIDFLOW_ENB\n");
	if (op & QIB_RCVCTRL_TIDFLOW_DIS)
		dd_dev_info(dd, "    QIB_RCVCTRL_TIDFLOW_DIS\n");

	rcvctrl = read_kctxt_csr(dd, ctxt, WFR_RCV_CTXT_CTRL);
	/* if the context already enabled, don't do the extra steps */
	if ((op & QIB_RCVCTRL_CTXT_ENB)
			&& !(rcvctrl & WFR_RCV_CTXT_CTRL_ENABLE_SMASK)) {
		dd_dev_info(dd, "rcd->rcvhdrqtailaddr_phys 0x%lx\n", (unsigned long)rcd->rcvhdrqtailaddr_phys);
		dd_dev_info(dd, "rcd->rcvhdrq_phys 0x%lx\n", (unsigned long)rcd->rcvhdrq_phys);

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

static u32 iblink_state(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd,
			"%s: DC interface not implemented, reporting %lld\n",
			__func__, (unsigned long long)default_link_state);
	return ppd->lstate;
}

static u8 ibphys_portstate(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd,
			"%s: DC interface not implemented, reporting %lld\n",
		__func__, (unsigned long long)IB_PHYSPORTSTATE_LINKUP);
	return IB_PHYSPORTSTATE_LINKUP;
}

static int ib_updown(struct qib_pportdata *ppd, int ibup, u64 ibcs)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: ppd 0x%p, ibup 0x%x, ibcs 0x%lx: not implemented\n", __func__, ppd, ibup, (unsigned long)ibcs);
	return 1; /* no other IB status change processing */
}

static int gpio_mod(struct hfi_devdata *dd, u32 out, u32 dir, u32 mask)
{
	if (print_unimplemented)
		dd_dev_info(dd, "%s: not implemented\n", __func__);
	/* return non-zero to indicate positive progress */
	return 1;
}

static int late_initreg(struct hfi_devdata *dd)
{
	if (print_unimplemented)
		dd_dev_info(dd, "%s: not implemented\n", __func__);
	return 0;
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

static void initvl15_bufs(struct hfi_devdata *dd)
{
	if (print_unimplemented)
		dd_dev_info(dd, "%s: not implemented\n", __func__);
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

	dd_dev_info(rcd->dd, "%s: setting up context %d\n", __func__, context);
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
			BUG();
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
	per_context = WFR_RXE_NUM_RECEIVE_ARRAY_ENTRIES / dd->num_rcv_contexts;
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

/* set TXE CSRs to chip reset defaults */
static void reset_txe(struct hfi_devdata *dd)
{
	int i;

	write_csr(dd, WFR_SEND_CTRL, 0);
	write_csr(dd, WFR_SEND_HIGH_PRIORITY_LIMIT, 0);
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
	for (i = 0; i < WFR_TXE_NUM_PRIORITIES; i++) {
		write_csr(dd, WFR_SEND_LOW_PRIORITY_LIST + (8*i), 0);
		write_csr(dd, WFR_SEND_HIGH_PRIORITY_LIST + (8*i), 0);
	}
	for (i = 0; i < WFR_TXE_NUM_CONTEXT_SET; i++)
		write_csr(dd, WFR_SEND_CONTEXT_SET_CTRL + (8*i), 0);
	for (i = 0; i < WFR_TXE_NUM_32_BIT_COUNTER; i++)
		write_csr(dd, WFR_SEND_COUNTER_ARRAY32 + (8*i), 0);
	for (i = 0; i < WFR_TXE_NUM_64_BIT_COUNTER; i++)
		write_csr(dd, WFR_SEND_COUNTER_ARRAY64 + (8*i), 0);
	write_csr(dd, WFR_SEND_CM_CTRL, 0);
	write_csr(dd, WFR_SEND_CM_GLOBAL_CREDIT, 0);
	write_csr(dd, WFR_SEND_CM_TIMER_CTRL, 0);
	for (i = 0; i < WFR_TXE_NUM_DATA_VL; i++)
		write_csr(dd, WFR_SEND_CM_CREDIT_VL + (8*i), 0);
	write_csr(dd, WFR_SEND_CM_CREDIT_VL15, 0);

	for (i = 0; i < dd->chip_send_contexts; i++) {
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CTRL, 0);
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CREDIT_CTRL, 0);
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CREDIT_RETURN_ADDR, 0);
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
 * Read chip sizes and then reset parts to sane, disabled,values.  We cannot
 * depend on the chip just being reset - a driver may be loaded and
 * unloaded many times.
 *
 * Sets:
 * dd->chip_rcv_contexts  - number of receive contexts supported by the chip
 * dd->chip_send_contexts - number PIO send contexts supported by the chip
 * dd->chip_pio_mem_size  - size of PIO send memory, in blocks
 * dd->chip_sdma_engines  - number of SDMA engines supported by the chip
 */
static void init_chip(struct hfi_devdata *dd)
{
	dd->chip_rcv_contexts = read_csr(dd, WFR_RCV_CONTEXTS);
	dd->chip_send_contexts = read_csr(dd, WFR_SEND_CONTEXTS);
	dd->chip_sdma_engines = read_csr(dd, WFR_SEND_DMA_ENGINES);
	dd->chip_pio_mem_size = read_csr(dd, WFR_SEND_PIO_MEM_SIZE);

	// FIXME:
	// o make sure all rcv contexts are disabled
	// o make sure all of SDMA engines are disabled
	// o make sure all interrupts are disabled
	// o reset all interrupt mappings back to default
	// o other...
	reset_txe(dd);


	init_partition_keys(dd);
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

void init_txe(struct hfi_devdata *dd)
{
	int i;

	/* enable all general PIO, SDMA, and Egress errors */
	write_csr(dd, WFR_SEND_PIO_ERR_MASK, ~0ull);
	write_csr(dd, WFR_SEND_DMA_ERR_MASK, ~0ull);
	write_csr(dd, WFR_SEND_EGRESS_ERR_MASK, ~0ull);

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
#define WFR_NUM_VL_PRE_CREDITS 200 /* arbitrary */
	for (i = 0; i < WFR_TXE_NUM_DATA_VL; i++) {
		write_csr(dd, WFR_SEND_CM_CREDIT_VL + (8 * i),
			(WFR_NUM_VL_PRE_CREDITS
				& WFR_SEND_CM_CREDIT_VL_DEDICATED_LIMIT_VL_MASK)
			<< WFR_SEND_CM_CREDIT_VL_DEDICATED_LIMIT_VL_SHIFT);
	}
	write_csr(dd, WFR_SEND_CM_CREDIT_VL15,
			(WFR_NUM_VL_PRE_CREDITS
			    & WFR_SEND_CM_CREDIT_VL15_DEDICATED_LIMIT_VL_MASK)
			<< WFR_SEND_CM_CREDIT_VL15_DEDICATED_LIMIT_VL_SHIFT);
	write_csr(dd, WFR_SEND_CM_GLOBAL_CREDIT,
			((WFR_NUM_VL_PRE_CREDITS * (WFR_TXE_NUM_DATA_VL+1))
				& WFR_SEND_CM_GLOBAL_CREDIT_GLOBAL_LIMIT_MASK)
			<< WFR_SEND_CM_GLOBAL_CREDIT_GLOBAL_LIMIT_SHIFT);

	/*
	 * Set the inital max length for VL15.  The largest is
	 *
	 * 	  64	max IB global packet (LRH+GRH+BTH+ICRC)
	 *	   8	UD header (DETH)
	 *	2048	2K MAD packet
	 *	----
	 *      2120 bytes
	 *
	 * Note, we're just writing the CSR instead of a read-modify-write
	 * because we "know" that at startup nothing else should be set.
	 */
#define MAX_MAD_PACKET_DW ((64 + 8 + 2048) / 4)
//FIXME: For now, allow all sizes for all non-VL15 VLs.  Initial testing
//does not have a FM to set the sizes.
#if 0
	write_csr(dd, WFR_SEND_LEN_CHECK1,
		(u64)MAX_MAD_PACKET_DW << WFR_SEND_LEN_CHECK1_LEN_VL15_SHIFT);
#else
	write_csr(dd, WFR_SEND_LEN_CHECK0,
		WFR_SEND_LEN_CHECK0_LEN_VL3_SMASK
		| WFR_SEND_LEN_CHECK0_LEN_VL2_SMASK
		| WFR_SEND_LEN_CHECK0_LEN_VL1_SMASK
		| WFR_SEND_LEN_CHECK0_LEN_VL0_SMASK);
	write_csr(dd, WFR_SEND_LEN_CHECK1,
		((u64)MAX_MAD_PACKET_DW  << WFR_SEND_LEN_CHECK1_LEN_VL15_SHIFT)
		| WFR_SEND_LEN_CHECK1_LEN_VL7_SMASK
		| WFR_SEND_LEN_CHECK1_LEN_VL6_SMASK
		| WFR_SEND_LEN_CHECK1_LEN_VL5_SMASK
		| WFR_SEND_LEN_CHECK1_LEN_VL4_SMASK);
#endif
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
	int i, mtu, ret;

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
		ppd[i].link_speed_enabled = ppd->link_speed_supported;

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

		/*
		 * Set the initial values to reasonable default, will be set
		 * for real when link is up.
		 */
		ppd[i].link_width_active = IB_WIDTH_4X;
		ppd[i].link_speed_active = IB_SPEED_EDR;
	}

	dd->f_bringup_serdes    = bringup_serdes;
	dd->f_cleanup           = cleanup;
	dd->f_clear_tids        = clear_tids;
	dd->f_free_irq          = stop_irq;
	dd->f_get_base_info     = get_base_info;
	dd->f_get_msgheader     = get_msgheader;
	dd->f_gpio_mod          = gpio_mod;
	dd->f_hdrqempty         = hdrqempty;
	dd->f_ib_updown         = ib_updown;
	dd->f_init_ctxt         = init_ctxt;
	dd->f_initvl15_bufs     = initvl15_bufs;
	dd->f_intr_fallback     = intr_fallback;
	dd->f_late_initreg      = late_initreg;
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
	dd->f_wantpiobuf_intr   = wantpiobuf_intr;
	dd->f_xgxs_reset        = xgxs_reset;
	dd->f_sdma_hw_clean_up  = sdma_hw_clean_up;
	dd->f_sdma_hw_start_up  = sdma_hw_start_up;
	dd->f_sdma_init_early   = sdma_init_early;
	dd->f_tempsense_rd	= tempsense_rd;
	/*
	 * Do remaining PCIe setup and save PCIe values in dd.
	 * Any error printing is already done by the init code.
	 * On return, we have the chip mapped.
	 */
	ret = qib_pcie_ddinit(dd, pdev, ent);
	if (ret < 0)
		goto bail_free;

	/* verify that reads actually work, save revision for reset check  */
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
	dd->hfi_id = (read_csr(dd, WFR_CCE_REVISION2) &
				WFR_CCE_REVISION2_HFI_ID_SMASK)
					>> WFR_CCE_REVISION2_HFI_ID_SHIFT;

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

	/* rcvegrbufsize must be set before calling qib_create_ctxts() */
	/* TODO: can ib_mtu_enum_to_int cover the full valid eager buffer
	  size range?  if not, we should drop using it and the ib enum */
	mtu = ib_mtu_enum_to_int(qib_ibmtu);
	if (mtu == -1)
		mtu = HFI_DEFAULT_MTU;
	/* quietly adjust the size to a valid range supported by the chip */
	dd->rcvegrbufsize = max(mtu,		          4 * 1024);
	dd->rcvegrbufsize = min(dd->rcvegrbufsize, (u32)128 * 1024);
	dd->rcvegrbufsize &= ~(4096ul - 1);	  /* remove lower bits */
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
