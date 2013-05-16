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

#include <linux/delay.h>

#include "hfi.h"


#define NUM_IB_PORTS 1
struct qib_chip_specific {
	u64 dummy;
};
struct qib_chippport_specific {
	u64 dummy;
};

static void qib_7322_sdma_sendctrl(struct qib_pportdata *ppd, unsigned op)
{
}

static void qib_7322_sdma_hw_clean_up(struct qib_pportdata *ppd)
{
}

static void qib_sdma_update_7322_tail(struct qib_pportdata *ppd, u16 tail)
{
}

static void qib_7322_sdma_hw_start_up(struct qib_pportdata *ppd)
{
}

static void qib_7322_set_intr_state(struct qib_devdata *dd, u32 enable)
{
	/*printk("%s: called\n", __func__);*/
}

static void qib_set_7322_armlaunch(struct qib_devdata *dd, u32 enable)
{
}

static int qib_7322_bringup_serdes(struct qib_pportdata *ppd)
{
	/*printk("%s: called\n", __func__);*/
	return 0;
}

static void qib_7322_mini_quiet_serdes(struct qib_pportdata *ppd)
{
}

static void qib_setup_7322_setextled(struct qib_pportdata *ppd, u32 on)
{
}

static void qib_7322_free_irq(struct qib_devdata *dd)
{
}

static void qib_setup_7322_cleanup(struct qib_devdata *dd)
{
}

static void qib_wantpiobuf_7322_intr(struct qib_devdata *dd, u32 needint)
{
}

static int qib_do_7322_reset(struct qib_devdata *dd)
{
	return 0;
}

static void qib_7322_put_tid(struct qib_devdata *dd, u64 __iomem *tidptr,
			     u32 type, unsigned long pa)
{
}

static void qib_7322_clear_tids(struct qib_devdata *dd,
				struct qib_ctxtdata *rcd)
{
}

static int qib_7322_get_base_info(struct qib_ctxtdata *rcd,
				  struct qib_base_info *kinfo)
{
	return 0;
}

static struct qib_message_header *qib_7322_get_msgheader(
				struct qib_devdata *dd, __le32 *rhf_addr)
{
	return (struct qib_message_header *)rhf_addr;
}

static int qib_7322_get_ib_cfg(struct qib_pportdata *ppd, int which)
{

	return 0;
}

static int qib_7322_set_ib_cfg(struct qib_pportdata *ppd, int which, u32 val)
{
	return 0;
}

static int qib_7322_set_loopback(struct qib_pportdata *ppd, const char *what)
{
	return 0;
}

static int qib_7322_get_ib_table(struct qib_pportdata *ppd, int which, void *t)
{
	return 0;
}

static int qib_7322_set_ib_table(struct qib_pportdata *ppd, int which, void *t)
{
	return 0;
}

static void qib_update_7322_usrhead(struct qib_ctxtdata *rcd, u64 hd,
				    u32 updegr, u32 egrhd, u32 npkts)
{
}

static u32 qib_7322_hdrqempty(struct qib_ctxtdata *rcd)
{
	return 0; /* not empty */
}

static void rcvctrl_7322_mod(struct qib_pportdata *ppd, unsigned int op,
			     int ctxt)
{
}

static void sendctrl_7322_mod(struct qib_pportdata *ppd, u32 op)
{
}

static u64 qib_portcntr_7322(struct qib_pportdata *ppd, u32 reg)
{
	return 0;
}

static u32 qib_read_7322cntrs(struct qib_devdata *dd, loff_t pos, char **namep,
			      u64 **cntrp)
{
	return 0; /* final read after getting everything */
}

static u32 qib_read_7322portcntrs(struct qib_devdata *dd, loff_t pos, u32 port,
				  char **namep, u64 **cntrp)
{
	return 0; /* final read after getting everything */
}

static void qib_get_7322_faststats(unsigned long opaque)
{
	struct qib_devdata *dd = (struct qib_devdata *) opaque;
	mod_timer(&dd->stats_timer, jiffies + HZ * ACTIVITY_TIMER);
}

static int qib_7322_intr_fallback(struct qib_devdata *dd)
{
	return 1;
}

static void qib_7322_mini_pcs_reset(struct qib_pportdata *ppd)
{
}

static u32 qib_7322_iblink_state(u64 ibcs)
{
	return IB_PORT_DOWN;
}

static u8 qib_7322_phys_portstate(u64 ibcs)
{
	return 0;
}

static int qib_7322_ib_updown(struct qib_pportdata *ppd, int ibup, u64 ibcs)
{
	return 1; /* no other IB status change processing */
}

static int gpio_7322_mod(struct qib_devdata *dd, u32 out, u32 dir, u32 mask)
{
	/* return non-zero to indicate positive progress */
	return 1;
}

static int qib_late_7322_initreg(struct qib_devdata *dd)
{
	return 0;
}

static int qib_init_7322_variables(struct qib_devdata *dd)
{
	struct qib_pportdata *ppd;

	/* pport structs are contiguous, allocated after devdata */
	ppd = (struct qib_pportdata *)(dd + 1);
	dd->pport = ppd;
	ppd[0].dd = dd;
	dd->cspec = (struct qib_chip_specific *)&ppd[NUM_IB_PORTS];
	ppd[0].cpspec = (struct qib_chippport_specific *)
						&dd->cspec[NUM_IB_PORTS];

	/* arbitrary values */
	dd->palign = 8;
	dd->uregbase = 0;
	dd->rcvtidcnt = 32;
	dd->rcvtidbase = 0;
	dd->rcvegrbase = 0;
	dd->piobufbase = 0;
	dd->pio2k_bufbase = 0;
	dd->piobcnt2k = 32;
	dd->piobcnt4k = 32;
	dd->piosize2k = 32;
	dd->piosize4k = 32;

	dd->boardname = kmalloc(64, GFP_KERNEL);
	sprintf(dd->boardname, "fake wfr");

	dd->num_pports = 1;

	/* setup the stats timer; the add_timer is done at end of init */
	init_timer(&dd->stats_timer);
	dd->stats_timer.function = qib_get_7322_faststats;
	dd->stats_timer.data = (unsigned long) dd;

	dd->ureg_align = 0x10000;  /* 64KB alignment */

	dd->piosize2kmax_dwords = dd->piosize2k >> 2;

	/* arbitrary values */
	dd->ctxtcnt = 32;
	dd->rcvhdrcnt = 32;

	qib_set_ctxtcnt(dd);

	return 0;
}

static u32 __iomem *qib_7322_getsendbuf(struct qib_pportdata *ppd, u64 pbc,
					u32 *pbufnum)
{
	return NULL;
}

static void qib_set_cntr_7322_sample(struct qib_pportdata *ppd, u32 intv,
				     u32 start)
{
}

static void qib_sdma_set_7322_desc_cnt(struct qib_pportdata *ppd, unsigned cnt)
{
}

static void qib_7322_sdma_init_early(struct qib_pportdata *ppd)
{
}

static int init_sdma_7322_regs(struct qib_pportdata *ppd)
{
	return 0;
}

static u16 qib_sdma_7322_gethead(struct qib_pportdata *ppd)
{
	return 0;
}

static int qib_sdma_7322_busy(struct qib_pportdata *ppd)
{
	return 0;
}

static u32 qib_7322_setpbc_control(struct qib_pportdata *ppd, u32 plen,
				   u8 srate, u8 vl)
{
	return 0;
}

static void qib_7322_initvl15_bufs(struct qib_devdata *dd)
{
}

static void qib_7322_init_ctxt(struct qib_ctxtdata *rcd)
{
}

static void qib_7322_txchk_change(struct qib_devdata *dd, u32 start,
				  u32 len, u32 which, struct qib_ctxtdata *rcd)
{
}

static void writescratch(struct qib_devdata *dd, u32 val)
{
}

static int qib_7322_tempsense_rd(struct qib_devdata *dd, int regnum)
{
	return -ENXIO;
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
struct qib_devdata *qib_init_wfr_funcs(struct pci_dev *pdev,
					   const struct pci_device_id *ent)
{
	struct qib_devdata *dd;
	int ret;

	dd = qib_alloc_devdata(pdev,
		NUM_IB_PORTS * sizeof(struct qib_pportdata) +
		sizeof(struct qib_chip_specific) +
		NUM_IB_PORTS * sizeof(struct qib_chippport_specific));
	if (IS_ERR(dd))
		goto bail;

	dd->f_bringup_serdes    = qib_7322_bringup_serdes;
	dd->f_cleanup           = qib_setup_7322_cleanup;
	dd->f_clear_tids        = qib_7322_clear_tids;
	dd->f_free_irq          = qib_7322_free_irq;
	dd->f_get_base_info     = qib_7322_get_base_info;
	dd->f_get_msgheader     = qib_7322_get_msgheader;
	dd->f_getsendbuf        = qib_7322_getsendbuf;
	dd->f_gpio_mod          = gpio_7322_mod;
	dd->f_hdrqempty         = qib_7322_hdrqempty;
	dd->f_ib_updown         = qib_7322_ib_updown;
	dd->f_init_ctxt         = qib_7322_init_ctxt;
	dd->f_initvl15_bufs     = qib_7322_initvl15_bufs;
	dd->f_intr_fallback     = qib_7322_intr_fallback;
	dd->f_late_initreg      = qib_late_7322_initreg;
	dd->f_setpbc_control    = qib_7322_setpbc_control;
	dd->f_portcntr          = qib_portcntr_7322;
	dd->f_put_tid           = qib_7322_put_tid;
	dd->f_quiet_serdes      = qib_7322_mini_quiet_serdes;
	dd->f_rcvctrl           = rcvctrl_7322_mod;
	dd->f_read_cntrs        = qib_read_7322cntrs;
	dd->f_read_portcntrs    = qib_read_7322portcntrs;
	dd->f_reset             = qib_do_7322_reset;
	dd->f_init_sdma_regs    = init_sdma_7322_regs;
	dd->f_sdma_busy         = qib_sdma_7322_busy;
	dd->f_sdma_gethead      = qib_sdma_7322_gethead;
	dd->f_sdma_sendctrl     = qib_7322_sdma_sendctrl;
	dd->f_sdma_set_desc_cnt = qib_sdma_set_7322_desc_cnt;
	dd->f_sdma_update_tail  = qib_sdma_update_7322_tail;
	dd->f_sendctrl          = sendctrl_7322_mod;
	dd->f_set_armlaunch     = qib_set_7322_armlaunch;
	dd->f_set_cntr_sample   = qib_set_cntr_7322_sample;
	dd->f_iblink_state      = qib_7322_iblink_state;
	dd->f_ibphys_portstate  = qib_7322_phys_portstate;
	dd->f_get_ib_cfg        = qib_7322_get_ib_cfg;
	dd->f_set_ib_cfg        = qib_7322_set_ib_cfg;
	dd->f_set_ib_loopback   = qib_7322_set_loopback;
	dd->f_get_ib_table      = qib_7322_get_ib_table;
	dd->f_set_ib_table      = qib_7322_set_ib_table;
	dd->f_set_intr_state    = qib_7322_set_intr_state;
	dd->f_setextled         = qib_setup_7322_setextled;
	dd->f_txchk_change      = qib_7322_txchk_change;
	dd->f_update_usrhead    = qib_update_7322_usrhead;
	dd->f_wantpiobuf_intr   = qib_wantpiobuf_7322_intr;
	dd->f_xgxs_reset        = qib_7322_mini_pcs_reset;
	dd->f_sdma_hw_clean_up  = qib_7322_sdma_hw_clean_up;
	dd->f_sdma_hw_start_up  = qib_7322_sdma_hw_start_up;
	dd->f_sdma_init_early   = qib_7322_sdma_init_early;
	dd->f_writescratch      = writescratch;
	dd->f_tempsense_rd	= qib_7322_tempsense_rd;
	/*
	 * Do remaining PCIe setup and save PCIe values in dd.
	 * Any error printing is already done by the init code.
	 * On return, we have the chip mapped, but chip registers
	 * are not set up until start of qib_init_7322_variables.
	 */
	ret = qib_pcie_ddinit(dd, pdev, ent);
	if (ret < 0)
		goto bail_free;

	/* initialize chip-specific variables */
	ret = qib_init_7322_variables(dd);
	if (ret)
		goto bail_cleanup;

	goto bail;

bail_cleanup:
	qib_pcie_ddcleanup(dd);
bail_free:
	qib_free_devdata(dd);
	dd = ERR_PTR(ret);
bail:
	return dd;
}
