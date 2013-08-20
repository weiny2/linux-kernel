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

#define NUM_IB_PORTS 1


static uint mod_num_sdma;
module_param_named(num_sdma, mod_num_sdma, uint, S_IRUGO);
MODULE_PARM_DESC(num_sdma, "Set max number SDMA engines to use");

/* TODO: temporary */
static uint print_unimplemented = 1;
module_param_named(print_unimplemented, print_unimplemented, uint, S_IRUGO);
MODULE_PARM_DESC(print_unimplemented, "Have unimplemented functions print when called");


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

u64 read_kctxt_csr(const struct hfi_devdata *dd, int ctxt, u32 offset0)
{
	/* kernel per-context CSRs are separated by 0x100 */
	return read_csr(dd, offset0 + (0x100* ctxt));
}

static void write_kctxt_csr(struct hfi_devdata *dd, int ctxt, u32 offset0,
		u64 value)
{
	/* kernel per-context CSRs are separated by 0x100 */
	write_csr(dd, offset0 + (0x100 * ctxt), value);
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

/* chip interrupt source table */
struct is_table {
	/* start of the interrupt source range */
	unsigned int start;
	/* routine that returns the name of the interrupt source */
	char *(*is_name)(char *name, size_t size, unsigned int source);
	/* routine to call from when receiving an interrupt */
	void (*is_int)(struct hfi_devdata *dd, unsigned int source);
};

static char *is_general_err_name(char *buf, size_t bsize, unsigned int source)
{
	const char *src;

	switch (source) {
	case 0: src = "CcePerHfiErrInt"; break;
	case 1: src = "RcvPerHfiErrInt"; break;
	case 4: src = "PioSendPerHfiErrInt"; break;
	case 5: src = "SDmaPerHfiErrInt"; break;
	case 6: src = "PacketEgressPerHfiErrInt"; break;
	case 2:
	case 3:
	case 7: src = "Reserved%u"; break;
	default: src = "invalid%u"; break;
	}
	snprintf(buf, bsize, src, source);
	return buf;
}

static char *is_rcvctxt_err_name(char *buf, size_t bsize, unsigned int source)
{
	snprintf(buf, bsize, "RcvCtxtErrInt%u", source);
	return buf;
}

static char *is_sendctxt_err_name(char *buf, size_t bsize, unsigned int source)
{
	snprintf(buf, bsize, "SendCtxtErrInt%u", source);
	return buf;
}

static char *is_sdma_err_name(char *buf, size_t bsize, unsigned int source)
{
	snprintf(buf, bsize, "SDmaErrInt%u", source);
	return buf;
}

static char *is_various_name(char *buf, size_t bsize, unsigned int source)
{
	/* TBD */
	snprintf(buf, bsize, "Various%u", source);
	return buf;
}

static char *is_rcvavailint_name(char *buf, size_t bsize, unsigned int source)
{
	snprintf(buf, bsize, "RcvAvailInt%u", source);
	return buf;
}

static char *is_sendcredit_name(char *buf, size_t bsize, unsigned int source)
{
	snprintf(buf, bsize, "SendCreditInt%u", source);
	return buf;
}

static char *is_sdmaint_name(char *buf, size_t bsize, unsigned int source)
{
	snprintf(buf, bsize, "SDmaInt%u", source);
	return buf;
}

static char *is_sdmaprogress_name(char *buf, size_t bsize, unsigned int source)
{
	snprintf(buf, bsize, "SDmaProgressInt%u", source);
	return buf;
}

static char *is_sdmaidle_name(char *buf, size_t bsize, unsigned int source)
{
	snprintf(buf, bsize, "SDmaIdleInt%u", source);
	return buf;
}

static char *is_sdmacleanup_name(char *buf, size_t bsize, unsigned int source)
{
	snprintf(buf, bsize, "SDmaCleanupDoneInt%u", source);
	return buf;
}

static void handle_sdma_interrupt(struct sdma_engine *per_sdma)
{
	/* TODO: actually do something */
	printk("%s: engine #%d - unimplemented\n", __func__, per_sdma->which);
}

static void is_general_err_int(struct hfi_devdata *dd, unsigned int source)
{
	/* TODO: actually do something */
	printk("%s: int%u - unimplemented\n", __func__ , source);
}

static void is_rcvctxt_err_int(struct hfi_devdata *dd, unsigned int source)
{
	/* TODO: actually do something */
	printk("%s: int%u - unimplemented\n", __func__ , source);
}

static void is_sendctxt_err_int(struct hfi_devdata *dd, unsigned int source)
{
	/* TODO: actually do something */
	printk("%s: int%u - unimplemented\n", __func__ , source);
}

static void is_sdma_err_int(struct hfi_devdata *dd, unsigned int source)
{
	/* TODO: actually do something */
	printk("%s: int%u - unimplemented\n", __func__ , source);
}

static void is_various_int(struct hfi_devdata *dd, unsigned int source)
{
	/* TODO: actually do something */
	printk("%s: int%u - unimplemented\n", __func__ , source);
}

static void is_rcvavailint_int(struct hfi_devdata *dd, unsigned int source)
{
	struct qib_ctxtdata *rcd;

	/* only check what we're using */
	if (source >= dd->num_rcv_contexts) {
		qib_dev_err(dd,
			"unexpected receive context interrupt %u\n", source);
		return;
	}

	rcd = dd->rcd[source];
	if (rcd)
		handle_receive_interrupt(rcd);
	else
		qib_dev_err(dd,
			"receive context interrupt %u, but no rcd\n", source);
}

static void is_sendcredit_int(struct hfi_devdata *dd, unsigned int source)
{
	/* TODO: actually do something */
	printk("%s: int%u - unimplemented\n", __func__ , source);
}

/* handles: sdmaint, sdmaprogressint, sdmaidleint */
static void is_fast_sdma_int(struct hfi_devdata *dd, unsigned int source)
{
	if (source < dd->num_sdma) {
		handle_sdma_interrupt(&dd->per_sdma[source]);
	} else {
		/* TODO: Which fast SDMA interrupt is this? */
		/* shouldn't happen */
		qib_dev_err(dd, "invalid fast SDMA interrupt - sdma%u\n",
			source);
	}
}

static void is_sdmacleanup_int(struct hfi_devdata *dd, unsigned int source)
{
	if (source < dd->num_sdma) {
		/* TODO: handle this */
		printk("%s: sdma%u - unimplemented\n", __func__, source);
	} else {
		/* shouldn't happen */
		qib_dev_err(dd, "invalid slow SDMA interrupt - sdma%u\n",
			source);
	}
}

static struct is_table is_table[] = {
{ WFR_IS_GENERAL_ERR_START,  is_general_err_name,  is_general_err_int  },
{ WFR_IS_RCVCTXT_ERR_START,  is_rcvctxt_err_name,  is_rcvctxt_err_int  },
{ WFR_IS_SENDCTXT_ERR_START, is_sendctxt_err_name, is_sendctxt_err_int },
{ WFR_IS_SDMA_ERR_START,     is_sdma_err_name,     is_sdma_err_int     },
{ WFR_IS_VAROUS_START,	     is_various_name,	   is_various_int      },
{ WFR_IS_RCVAVAILINT_START,  is_rcvavailint_name,  is_rcvavailint_int  },
{ WFR_IS_SENDCREDIT_START,   is_sendcredit_name,   is_sendcredit_int   },
{ WFR_IS_SDMAINT_START,	     is_sdmaint_name,	   is_fast_sdma_int    },
{ WFR_IS_SDMAPROGRESS_START, is_sdmaprogress_name, is_fast_sdma_int    },
{ WFR_IS_SDMAIDLE_START,     is_sdmaidle_name,	   is_fast_sdma_int    },
{ WFR_IS_SDMACLEANUP_START,  is_sdmacleanup_name,  is_sdmacleanup_int  },
{ WFR_IS_MAX_SOURCES, 	     NULL,		   NULL		       }
};

/*
 * Interrupt source name - return the buffer with the text name
 * of the interrupt source.
 */
static char *is_name(char *buf, size_t bsize, unsigned int source)
{
	struct is_table *entry;

	/* avoids a double compare by walking the table in-order */
	for (entry = &is_table[0]; entry->is_name; entry++) {
		if (source < entry[1].start)
			return entry->is_name(buf, bsize, source-entry->start);
	}
	/* fell off the end */
	snprintf(buf, bsize, "invalid interrupt source %u\n", source);
	return buf;
}

/*
 * Interupt source interrupt - called when the given source has
 * an interrupt.
 */
static void is_interrupt(struct hfi_devdata *dd, unsigned int source)
{
	struct is_table *entry;

	/* avoids a double compare by walking the table in-order */
	for (entry = &is_table[0]; entry->is_name; entry++) {
		if (source < entry[1].start) {
			entry->is_int(dd, source-entry->start);
			return;
		}
	}
	/* fell off the end */
	qib_dev_err(dd, "invalid interrupt source %u\n", source);
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
		printk(DRIVER_NAME"%d: interrupt: %d, %s\n", dd->unit, bit,
			is_name(buf, sizeof(buf), bit));
		is_interrupt(dd, bit);
	}

	return IRQ_HANDLED;
}

static irqreturn_t sdma_interrupt(int irq, void *data)
{
	struct sdma_engine *per_sdma = data;

	per_sdma->dd->int_counter++;

	/* clear the interrupt */
	write_csr(per_sdma->dd,
		WFR_CCE_INT_CLEAR + (8*(WFR_IS_SDMAINT_START/64)),
		per_sdma->imask);

	/* handle the interrupt */
	handle_sdma_interrupt(per_sdma);

	return IRQ_HANDLED;
}

static irqreturn_t receive_context_interrupt(int irq, void *data)
{
	struct qib_ctxtdata *rcd = data;

	rcd->dd->int_counter++;

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
	u64 reg;

	if (print_unimplemented)
		dd_dev_info(dd, "%s: ppd 0x%p: not implemented (enabling port)\n", __func__, ppd);

	/* enable the port */
	/* NOTE: 7322: done within rcvmod_lock */
	reg = read_csr(dd, WFR_RCV_CTRL);
	reg |= WFR_RCV_CTRL_RCV_PORT_ENABLE_SMASK;
	write_csr(dd, WFR_RCV_CTRL, reg);
	/* TODO: clear RcvCtrl.RcvPortEnable in quiet_serdes()? */

	/* 7322: enable the serdes status change interrupt */

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
		qib_dev_err(dd,
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
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: which %s, val 0x%x: not implemented\n", __func__, ib_cfg_name(which), val);
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
static void one_rcvctrl(struct hfi_devdata *dd, unsigned int op, int ctxt)
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

static void rcvctrl(struct hfi_devdata *dd, unsigned int op, int ctxt)
{
	int i;

// FIXME: it looks like a -1 means "all contexts for this port"
// Since we only have one port, we can safely do all.  However
// this begs the question: Do we remove the notion of multiple
// ports?  There are several functions that take a ppd when
// it is now no longer needed NOR applicable.
	if (ctxt < 0) {
		for (i = 0; i < dd->num_rcv_contexts; i++)
			one_rcvctrl(dd, op, i);
	} else {
		one_rcvctrl(dd, op, ctxt);
	}
}

static char *sendctrl_op_str(unsigned op, char *buf, int bufsize)
{
	char *p = buf;

	if (op & QIB_SENDCTRL_DISARM) {
		unsigned bn = op & 0xfff;
		p += snprintf(p, bufsize - (p-buf), "DISARM %d,", bn);
		op &= ~(QIB_SENDCTRL_DISARM | 0xfff);
	}
	if (op & QIB_SENDCTRL_AVAIL_DIS) {
		p += snprintf(p, bufsize - (p-buf), "AVAIL_DIS,");
		op &= ~QIB_SENDCTRL_AVAIL_DIS;
	}
	if (op & QIB_SENDCTRL_AVAIL_ENB) {
		p += snprintf(p, bufsize - (p-buf), "AVAIL_ENB,");
		op &= ~QIB_SENDCTRL_AVAIL_ENB;
	}
	if (op & QIB_SENDCTRL_AVAIL_BLIP) {
		p += snprintf(p, bufsize - (p-buf), "AVAIL_BLIP,");
		op &= ~QIB_SENDCTRL_AVAIL_BLIP;
	}
	if (op & QIB_SENDCTRL_SEND_DIS) {
		p += snprintf(p, bufsize - (p-buf), "SEND_DIS,");
		op &= ~QIB_SENDCTRL_SEND_DIS;
	}
	if (op & QIB_SENDCTRL_SEND_ENB) {
		p += snprintf(p, bufsize - (p-buf), "SEND_ENB,");
		op &= ~QIB_SENDCTRL_SEND_ENB;
	}
	if (op & QIB_SENDCTRL_FLUSH) {
		p += snprintf(p, bufsize - (p-buf), "FLUSH,");
		op &= ~QIB_SENDCTRL_FLUSH;
	}
	if (op & QIB_SENDCTRL_CLEAR) {
		p += snprintf(p, bufsize - (p-buf), "CLEAR,");
		op &= ~QIB_SENDCTRL_CLEAR;
	}
	if (op & QIB_SENDCTRL_DISARM_ALL) {
		p += snprintf(p, bufsize - (p-buf), "DISARM_ALL,");
		op &= ~QIB_SENDCTRL_DISARM_ALL;
	}
	/* any bits left? */
	if (op) {
		p += snprintf(p, bufsize - (p-buf), "0x%x,", op);
	}

	return buf;
}

static void sendctrl(struct qib_pportdata *ppd, u32 op)
{
	char buf[128];

	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: op [%s] - not implemented\n",
			__func__, sendctrl_op_str(op, buf, sizeof(buf)));
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

static u32 iblink_state(u64 ibcs)
{
	printk("%s: ibcs 0x%lx: not implemented, reporting IB_PORT_ACTIVE\n", __func__, (unsigned long)ibcs);
	return IB_PORT_ACTIVE;
}

static u8 ibphys_portstate(u64 ibcs)
{
	printk("%s: ibcs 0x%lx: not implemented, reporting IB_PHYSPORTSTATE_LINKUP\n", __func__, (unsigned long)ibcs);
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

static u32 __iomem *getsendbuf(struct qib_pportdata *ppd, u64 pbc,
					u32 *pbufnum)
{
	if (print_unimplemented)
		dd_dev_info(ppd->dd, "%s: not implemented\n", __func__);
	return NULL;
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

static u32 setpbc_control(struct qib_pportdata *ppd, u32 plen,
				   u8 srate, u8 vl)
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

	dd_dev_info(rcd->dd, "%s: setting up context %d\n", __func__, rcd->ctxt);
	/*
	 * Simple allocation: we have already pre-allocated the number
	 * of entries per context in dd->rcv_entries.  Now, divide the
	 * per context value by 2 and use half for eager and expected.
	 * This requires that the count be divisible by 4 as the expected
	 * array base and count must be divisible by 2.
	 */
	rcd->eager_count = dd->rcv_entries / 2;
	rcd->expected_count = dd->rcv_entries / 2;
	rcd->eager_base = (dd->rcv_entries * rcd->ctxt);
	rcd->expected_base = rcd->eager_base + rcd->eager_count;
	BUG_ON(rcd->expected_base % 2 == 1);	/* must be even */

#if 0
	if (rcd->ctxt < rcd->dd->first_user_ctxt) {
		/* a kernel context */
	} else {
		/* a user context */
	}
#endif
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
	write_kctxt_csr(dd, rcd->ctxt, WFR_RCV_HDR_CNT, reg);
	reg = (dd->rcvhdrentsize & WFR_RCV_HDR_ENT_SIZE_ENT_SIZE_MASK)
		<< WFR_RCV_HDR_ENT_SIZE_ENT_SIZE_SHIFT;
	write_kctxt_csr(dd, rcd->ctxt, WFR_RCV_HDR_ENT_SIZE, reg);
	reg = (dd->rcvhdrsize & WFR_RCV_HDR_SIZE_HDR_SIZE_MASK)
		<< WFR_RCV_HDR_SIZE_HDR_SIZE_SHIFT;
	write_kctxt_csr(dd, rcd->ctxt, WFR_RCV_HDR_SIZE, reg);
}

static void txchk_change(struct hfi_devdata *dd, u32 start,
				  u32 len, u32 which, struct qib_ctxtdata *rcd)
{
	if (print_unimplemented)
		dd_dev_info(dd, "%s: not implemented\n", __func__);
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
	 * engine.  Per-engine interrupts are as follows, we want the
	 * first 3:
	 *	SDMAInt		- fast path
	 *	SDMAProgressInt - fast path
	 *	SDMAIdleInt	- fast path
	 *	SDMACleanupDone	- slow path
	 */
	remap_intr(dd, WFR_IS_SDMAINT_START      + engine, msix_intr);
	remap_intr(dd, WFR_IS_SDMAPROGRESS_START + engine, msix_intr);
	remap_intr(dd, WFR_IS_SDMAIDLE_START     + engine, msix_intr);
}

static void remap_receive_context_interrupt(struct hfi_devdata *dd,
						int rx, int msix_intr)
{
	remap_intr(dd, WFR_IS_RCVAVAILINT_START + rx, msix_intr);
}

static int request_intx_irq(struct hfi_devdata *dd)
{
	int ret;

	snprintf(dd->intx_name, sizeof(dd->intx_name), DRIVER_NAME"%d",
		dd->unit);
	ret = request_irq(dd->pcidev->irq, general_interrupt,
				  IRQF_SHARED, dd->intx_name, dd);
	if (ret)
		qib_dev_err(dd, "unable to request INTx interrupt, err %d\n",
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
	 * Sanity check - the code expects all fast SDMA chip source
	 * interrupts to be in the same CSR.  Verify that this is true
	 * by checking the boundaries of the fast SDMA source interrupts.
	 */
	if ((WFR_IS_SDMAINT_START / 64) != (WFR_IS_SDMAIDLE_END-1) / 64) {
		qib_dev_err(dd, "SDMA interrupt sources not on same CSR");
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
			 * mapped here.  We have checked above that they
			 * are all on the same CSR.
			 */
			per_sdma->imask = 
				(u64)1 << ((WFR_IS_SDMAINT_START+idx)%64)
				| (u64)1 << ((WFR_IS_SDMAPROGRESS_START+idx)%64)
				| (u64)1 << ((WFR_IS_SDMAIDLE_START+idx)%64);
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
			rcd->ireg = (WFR_IS_RCVAVAILINT_START+idx) / 64;
			rcd->imask = ((u64)1) <<
					((WFR_IS_RCVAVAILINT_START+idx) % 64);
			handler = receive_context_interrupt;
			arg = rcd;
			snprintf(me->name, sizeof(me->name),
				DRIVER_NAME"%d kctxt%d", dd->unit, idx);
			err_info = "receive context";
			remap_receive_context_interrupt(dd, idx, i);
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
			qib_dev_err(dd,
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
	 *	N interrupts - one per used SDMA engine (handles the 3
	 *		fast sources)
	 *	M interrupt - one per used rx context
	 *	TODO: pio (tx) contexts?
	 */
	total = 1 + dd->num_sdma + dd->num_rcv_contexts;

	entries = kzalloc(sizeof(*entries) * total, GFP_KERNEL);
	if (!entries) {
		qib_dev_err(dd, "cannot allocate msix table\n");
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
	} else if (request == total) {
		/* using MSI-X */
		dd->num_msix_entries = total;
		dd->msix_entries = entries;
		dd_dev_info(dd, "%u MSI-X interrupts allocated\n", total);
	} else {
		/* using MSI-X, with reduced interrupts */
		/* TODO: handle reduced interrupt case?  Need scheme to
		   decide who shares. */
		qib_dev_err(dd,
			"cannot handle reduced interrupt case, want %u, got %u\n",
			total, request);
		ret = -EINVAL;
		goto fail;
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
 *	chip_rcv_contexts - total number of contexts supported by the chip
 *	num_rcv_contexts - number of contexts being used
 *	n_krcv_queues - number of kernel contexts
 *	first_user_ctxt - first non-kernel context in array of contexts
 */
static int set_up_context_variables(struct hfi_devdata *dd)
{
	int num_kernel_contexts;
	int num_user_contexts;
	int total_contexts;
	u32 chip_rx_contexts;
	u32 per_context;

	chip_rx_contexts = (u32)read_csr(dd, WFR_RCV_CONTEXTS);

	/*
	 * Kernel contexts: (to be fixed later):
	 * 	- default to 1 kernel context per CPU
	 */
	if (qib_n_krcv_queues) {
		num_kernel_contexts = qib_n_krcv_queues;
	} else {
		num_kernel_contexts = num_online_cpus();
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

	if (total_contexts > chip_rx_contexts) {
		/* don't silently adjust, complain and fail */
		qib_dev_err(dd,
			"not enough physical contexts: want %d, have %d\n",
			(int)total_contexts, (int)chip_rx_contexts);
		return -ENOSPC;
	}

	/* the first N are kernel contexts, the rest are user contexts */
	dd->chip_rcv_contexts = chip_rx_contexts;
	dd->num_rcv_contexts = total_contexts;
	dd->n_krcv_queues = num_kernel_contexts;
	dd->first_user_ctxt = num_kernel_contexts;
	dd->freectxts = num_user_contexts;
	dd_dev_info(dd,
		"rcv: chip contexts %d, num contexts %d, kernel contexts %d\n",
		(int)dd->chip_rcv_contexts, (int)dd->num_rcv_contexts, (int)dd->n_krcv_queues);

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
	/* FIXME: no more than 8 each of eager and expected TIDs, for now! */
	if (per_context > 16)
		per_context = 16;
	dd->rcv_entries = per_context;
	dd_dev_info(dd, "rcv entries %u\n", dd->rcv_entries);

	return 0;
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
		(0xffff & WFR_RCV_PARTITION_KEY_PARTITION_KEY_A_MASK)
			<< WFR_RCV_PARTITION_KEY_PARTITION_KEY_A_SHIFT);
	write_csr(dd, WFR_RCV_PARTITION_KEY + (1 * 8),  0);
	write_csr(dd, WFR_RCV_PARTITION_KEY + (2 * 8),  0);
	write_csr(dd, WFR_RCV_PARTITION_KEY + (3 * 8),  0);
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

	/* pport structs are contiguous, allocated after devdata */
	ppd = (struct qib_pportdata *)(dd + 1);
	dd->pport = ppd;
	for (i = 0; i < NUM_IB_PORTS; i++)
		ppd[i].dd = dd;

	dd->f_bringup_serdes    = bringup_serdes;
	dd->f_cleanup           = cleanup;
	dd->f_clear_tids        = clear_tids;
	dd->f_free_irq          = stop_irq;
	dd->f_get_base_info     = get_base_info;
	dd->f_get_msgheader     = get_msgheader;
	dd->f_getsendbuf        = getsendbuf;
	dd->f_gpio_mod          = gpio_mod;
	dd->f_hdrqempty         = hdrqempty;
	dd->f_ib_updown         = ib_updown;
	dd->f_init_ctxt         = init_ctxt;
	dd->f_initvl15_bufs     = initvl15_bufs;
	dd->f_intr_fallback     = intr_fallback;
	dd->f_late_initreg      = late_initreg;
	dd->f_setpbc_control    = setpbc_control;
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
	dd->f_sendctrl          = sendctrl;
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
	dd->f_txchk_change      = txchk_change;
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
		qib_dev_err(dd, "cannot read chip CSRs\n");
		goto bail_free;
	}

	dd->majrev = (dd->revision >> WFR_CCE_REVISION_CHIP_REV_MAJOR_SHIFT)
			& WFR_CCE_REVISION_CHIP_REV_MAJOR_MASK;
	dd->minrev = (dd->revision >> WFR_CCE_REVISION_CHIP_REV_MINOR_SHIFT)
			& WFR_CCE_REVISION_CHIP_REV_MINOR_MASK;

	/* read chip values */
	dd->chip_sdma_engines = read_csr(dd, WFR_SEND_DMA_ENGINES);
	dd->num_sdma = dd->chip_sdma_engines;
	if (mod_num_sdma && mod_num_sdma < dd->chip_sdma_engines)
		dd->num_sdma = mod_num_sdma;

	init_partition_keys(dd);

	/* FIXME: don't set NODMA_RTAIL until we know the sequence numbers
	   are right */
	//dd->flags = QIB_NODMA_RTAIL;

	dd->palign = 0x1000;	// TODO: is there a WFR value for this?

	/* FIXME: fill in with real values */
	dd->piobufbase = 0;
	dd->pio2k_bufbase = 0;
	dd->piobcnt2k = 32;
	dd->piobcnt4k = 32;
	dd->piosize2k = 32;
	dd->piosize2kmax_dwords = dd->piosize2k >> 2;
	dd->piosize4k = 32;

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
	/* quietly adjust the size to a valid range */
        dd->rcvegrbufsize = max(mtu,   4 * 1024); /* min size */
        dd->rcvegrbufsize = min(mtu, 128 * 1024); /* max size */
	dd->rcvegrbufsize &= ~(4096ul - 1);	  /* remove lower bits */

	/* TODO: real board name */
	dd->boardname = kmalloc(64, GFP_KERNEL);
	sprintf(dd->boardname, "fake wfr");

	/* only 1 physical port */
	dd->num_pports = 1;

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

	ret = qib_create_ctxts(dd);
	if (ret)
		goto bail_cleanup;

	/* use contexts created by qib_create_ctxts */
	ret = set_up_interrupts(dd);
	if (ret)
		goto bail_cleanup;

	ret = load_firmware(dd);
	if (ret)
		goto bail_cleanup;

	/*
	 * TODO: RX init, TX init
	 * 
	 * Set the CSRs to sane, expected values - the driver can be
	 * loaded and unloaded, we can't expect reset values.
	 */

	goto bail;

bail_cleanup:
	qib_pcie_ddcleanup(dd);
bail_free:
	qib_free_devdata(dd);
	dd = ERR_PTR(ret);
bail:
	return dd;
}
