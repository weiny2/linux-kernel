/*
 * Copyright (c) 2013 Intel Corporation. All rights reserved.
 * Copyright (c) 2006, 2007, 2008, 2009 QLogic Corporation. All rights reserved.
 * Copyright (c) 2003, 2004, 2005, 2006 PathScale, Inc. All rights reserved.
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

#include <linux/spinlock.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/prefetch.h>

#include "hfi.h"
#include "trace.h"
#include "qp.h"

/*
 * The size has to be longer than this string, so we can append
 * board/chip information to it in the init code.
 */
const char ib_qib_version[] = HFI_DRIVER_VERSION "\n";

DEFINE_SPINLOCK(qib_devs_lock);
LIST_HEAD(qib_dev_list);
DEFINE_MUTEX(qib_mutex);	/* general driver use */

unsigned int max_mtu;
module_param_named(max_mtu, max_mtu, uint, S_IRUGO);
MODULE_PARM_DESC(max_mtu, "Set max MTU bytes, default is 10240");

unsigned int default_mtu;
module_param_named(default_mtu, default_mtu, uint, S_IRUGO);
MODULE_PARM_DESC(max_mtu, "Set default MTU bytes, default is 4096");

unsigned int hfi_cu = 1;
module_param_named(cu, hfi_cu, uint, S_IRUGO);
MODULE_PARM_DESC(cu, "Credit return units");

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel <ibsupport@intel.com>");
MODULE_DESCRIPTION("Intel IB driver");
MODULE_VERSION(HFI_DRIVER_VERSION);

/*
 * MAX_PKT_RCV is the max # if packets processed per receive interrupt.
 */
#define MAX_PKT_RECV 64

struct qlogic_ib_stats qib_stats;

const char *get_unit_name(int unit)
{
	static char iname[16];

	snprintf(iname, sizeof iname, DRIVER_NAME"%u", unit);
	return iname;
}

/*
 * Return count of units with at least one port ACTIVE.
 */
int qib_count_active_units(void)
{
	struct hfi_devdata *dd;
	struct qib_pportdata *ppd;
	unsigned long flags;
	int pidx, nunits_active = 0;

	spin_lock_irqsave(&qib_devs_lock, flags);
	list_for_each_entry(dd, &qib_dev_list, list) {
		if (!(dd->flags & QIB_PRESENT) || !dd->kregbase)
			continue;
		for (pidx = 0; pidx < dd->num_pports; ++pidx) {
			ppd = dd->pport + pidx;
			if (ppd->lid && ppd->linkup) {
				nunits_active++;
				break;
			}
		}
	}
	spin_unlock_irqrestore(&qib_devs_lock, flags);
	return nunits_active;
}

/*
 * Return count of all units, optionally return in arguments
 * the number of usable (present) units, and the number of
 * ports that are up.
 */
int qib_count_units(int *npresentp, int *nupp)
{
	int nunits = 0, npresent = 0, nup = 0;
	struct hfi_devdata *dd;
	unsigned long flags;
	int pidx;
	struct qib_pportdata *ppd;

	spin_lock_irqsave(&qib_devs_lock, flags);

	list_for_each_entry(dd, &qib_dev_list, list) {
		nunits++;
		if ((dd->flags & QIB_PRESENT) && dd->kregbase)
			npresent++;
		for (pidx = 0; pidx < dd->num_pports; ++pidx) {
			ppd = dd->pport + pidx;
			if (ppd->lid && ppd->linkup)
				nup++;
		}
	}

	spin_unlock_irqrestore(&qib_devs_lock, flags);

	if (npresentp)
		*npresentp = npresent;
	if (nupp)
		*nupp = nup;

	return nunits;
}

/**
 * qib_wait_linkstate - wait for an IB link state change to occur
 * @ppd: port device
 * @state: the state to wait for
 * @msecs: the number of milliseconds to wait
 *
 * Wait up to msecs milliseconds for IB link state change to occur.
 * For now, take the easy polling route.
 * Returns 0 if state reached, otherwise -ETIMEDOUT.
 */
int qib_wait_linkstate(struct qib_pportdata *ppd, u32 state, int msecs)
{
	unsigned long timeout;

	timeout = jiffies + msecs_to_jiffies(msecs);
	while (1) {
		if (ppd->dd->f_iblink_state(ppd) == state)
			return 0;
		if (time_after(jiffies, timeout))
			break;
		msleep(2);
	}
	dd_dev_err(ppd->dd, "timeout waiting for link state 0x%x\n", state);

	return -ETIMEDOUT;
}

int qib_set_linkstate(struct qib_pportdata *ppd, u8 newstate)
{
	struct hfi_devdata *dd = ppd->dd;
	int set_ib_ready = 0;
	u32 phys_command;
	u32 lstate;
	int ret;

	/* update cached value */
	dd->f_iblink_state(ppd);

	switch (newstate) {
	case QIB_IB_LINKDOWN_ONLY:
		phys_command = IB_LINKINITCMD_NOP;
		goto down_common;

	case QIB_IB_LINKDOWN:
		phys_command = IB_LINKINITCMD_POLL;
		goto down_common;

	case QIB_IB_LINKDOWN_SLEEP:
		phys_command = IB_LINKINITCMD_SLEEP;
		goto down_common;

	case QIB_IB_LINKDOWN_DISABLE:
		phys_command = IB_LINKINITCMD_DISABLE;
		/* fall through */
down_common:
		dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LSTATE,
				 IB_LINKCMD_DOWN | phys_command);
		if (ppd->statusp)
			*ppd->statusp &= ~QIB_STATUS_IB_READY;
		lstate = IB_PORT_DOWN;
		break;

	case QIB_IB_LINKARM:
		if (ppd->lstate == IB_PORT_ARMED) {
			ret = 0;
			goto bail;
		}
		if (ppd->lstate != IB_PORT_INIT) {
			ret = -EINVAL;
			goto bail;
		}
		dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LSTATE,
				 IB_LINKCMD_ARMED | IB_LINKINITCMD_NOP);
		lstate = IB_PORT_ARMED;
		break;

	case QIB_IB_LINKACTIVE:
		if (ppd->lstate == IB_PORT_ACTIVE) {
			ret = 0;
			goto bail;
		}
		if (ppd->lstate != IB_PORT_ARMED) {
			ret = -EINVAL;
			goto bail;
		}
		dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LSTATE,
				 IB_LINKCMD_ACTIVE | IB_LINKINITCMD_NOP);
		lstate = IB_PORT_ACTIVE;
		set_ib_ready = 1;
		break;

	default:
		ret = -EINVAL;
		goto bail;
	}

	ret = qib_wait_linkstate(ppd, lstate, 10);
	if (ret == 0 && set_ib_ready) {
		/* only set if the state really moved */
		if (ppd->statusp)
			*ppd->statusp |= QIB_STATUS_IB_READY;
	}

bail:
	return ret;
}

/*
 * Get address of eager buffer from it's index (allocated in chunks, not
 * contiguous).
 */
static inline void *qib_get_egrbuf(const struct qib_ctxtdata *rcd, u32 etail)
{
	const u32 chunk = etail >> rcd->rcvegrbufs_perchunk_shift;
	const u32 idx =  etail & ((u32)rcd->rcvegrbufs_perchunk - 1);

	return rcd->rcvegrbuf[chunk] + (idx << rcd->dd->rcvegrbufsize_shift);
}

/*
 */
static void rcv_hdrerr(struct qib_ctxtdata *rcd, struct qib_pportdata *ppd,
			  u32 ctxt, u32 eflags, u32 l, u32 etail,
			  __le32 *rhf_addr, struct qib_message_header *rhdr)
{
	/*u32 rcv_type = rhf_rcv_type(rhf_addr);*/

	if (eflags & (RHF1_VCRC_ERR | RHF1_ICRC_ERR))
		return;

	/*
	 * TODO: In WFR, RHF.TIDErr can mean 2 things, depending on the
	 * packet type.  Does this code apply to both?
	 * For type 0 packets (expected receive), see section 8.5.6.2
	 * Expected TIDErr.
	 * For non-type 0 packets (not expected receive) see the section
	 * 8.5.7.3 Eager TIDErr, in HAS snapshots on or later than 2013-07-12.
	 */
	if (eflags & RHF1_TID_ERR) {
		/* For TIDERR and RC QPs premptively schedule a NAK */
		struct qib_ib_header *hdr = (struct qib_ib_header *) rhdr;
		struct qib_other_headers *ohdr = NULL;
		struct qib_ibport *ibp = &ppd->ibport_data;
		struct qib_qp *qp = NULL;
		u32 tlen = rhf_pkt_len(rhf_addr); /* in bytes */
		u16 lid  = be16_to_cpu(hdr->lrh[1]);
		int lnh = be16_to_cpu(hdr->lrh[0]) & 3;
		u32 qp_num;
		u32 opcode;
		u32 psn;
		int diff;

		/* Sanity check packet */
		if (tlen < 24)
			goto drop;

		if (lid < QIB_MULTICAST_LID_BASE) {
			lid &= ~((1 << ppd->lmc) - 1);
			if (unlikely(lid != ppd->lid))
				goto drop;
		}

		/* Check for GRH */
		if (lnh == QIB_LRH_BTH)
			ohdr = &hdr->u.oth;
		else if (lnh == QIB_LRH_GRH) {
			u32 vtf;

			ohdr = &hdr->u.l.oth;
			if (hdr->u.l.grh.next_hdr != IB_GRH_NEXT_HDR)
				goto drop;
			vtf = be32_to_cpu(hdr->u.l.grh.version_tclass_flow);
			if ((vtf >> IB_GRH_VERSION_SHIFT) != IB_GRH_VERSION)
				goto drop;
		} else
			goto drop;

		/* Get opcode and PSN from packet */
		opcode = be32_to_cpu(ohdr->bth[0]);
		opcode >>= 24;
		psn = be32_to_cpu(ohdr->bth[2]);

		/* Get the destination QP number. */
		qp_num = be32_to_cpu(ohdr->bth[1]) & QIB_QPN_MASK;
		if (qp_num != QIB_MULTICAST_QPN) {
			int ruc_res;
			qp = qib_lookup_qpn(ibp, qp_num);
			if (!qp)
				goto drop;

			/*
			 * Handle only RC QPs - for other QP types drop error
			 * packet.
			 */
			spin_lock(&qp->r_lock);

			/* Check for valid receive state. */
			if (!(ib_qib_state_ops[qp->state] &
			      QIB_PROCESS_RECV_OK)) {
				ibp->n_pkt_drops++;
				goto unlock;
			}

			switch (qp->ibqp.qp_type) {
			case IB_QPT_RC:
				ruc_res =
					qib_ruc_check_hdr(
						ibp, hdr,
						lnh == QIB_LRH_GRH,
						qp,
						be32_to_cpu(ohdr->bth[0]));
				if (ruc_res)
					goto unlock;

				/* Only deal with RDMA Writes for now */
				if (opcode <
				    IB_OPCODE_RC_RDMA_READ_RESPONSE_FIRST) {
					diff = qib_cmp24(psn, qp->r_psn);
					if (!qp->r_nak_state && diff >= 0) {
						ibp->n_rc_seqnak++;
						qp->r_nak_state =
							IB_NAK_PSN_ERROR;
						/* Use the expected PSN. */
						qp->r_ack_psn = qp->r_psn;
						/*
						 * Wait to send the sequence
						 * NAK until all packets
						 * in the receive queue have
						 * been processed.
						 * Otherwise, we end up
						 * propagating congestion.
						 */
						if (list_empty(&qp->rspwait)) {
							qp->r_flags |=
								QIB_R_RSP_NAK;
							atomic_inc(
								&qp->refcount);
							list_add_tail(
							 &qp->rspwait,
							 &rcd->qp_wait_list);
						}
					} /* Out of sequence NAK */
				} /* QP Request NAKs */
				break;
			case IB_QPT_SMI:
			case IB_QPT_GSI:
			case IB_QPT_UD:
			case IB_QPT_UC:
			default:
				/* For now don't handle any other QP types */
				break;
			}

unlock:
			spin_unlock(&qp->r_lock);
			/*
			 * Notify qib_destroy_qp() if it is waiting
			 * for us to finish.
			 */
			if (atomic_dec_and_test(&qp->refcount))
				wake_up(&qp->wait);
		} /* Unicast QP */
	} /* Valid packet with TIDErr */

drop:
	return;
}

/*
 * handle_receive_interrupt - receive a packet
 * @rcd: the context
 *
 * Called from interrupt handler for errors or receive interrupt.
 */
void handle_receive_interrupt(struct qib_ctxtdata *rcd)
{
	struct hfi_devdata *dd = rcd->dd;
	struct qib_pportdata *ppd = rcd->ppd;
	__le32 *rhf_addr;
	void *ebuf;
	const u32 rsize = dd->rcvhdrentsize;        /* words */
	const u32 maxcnt = dd->rcvhdrcnt * rsize;   /* words */
	u32 etail = -1, l, hdrqtail;
	struct qib_message_header *hdr;
	u32 eflags, etype, tlen, i = 0, updegr = 0;
	int last;
	u64 lval;
	struct qib_qp *qp, *nqp;

	l = rcd->head;
	rhf_addr = (__le32 *) rcd->rcvhdrq + l + dd->rhf_offset;
	if (dd->flags & QIB_NODMA_RTAIL) {
		u32 seq = rhf_rcv_seq(rhf_addr);
		if (seq != rcd->seq_cnt)
			goto bail;
		hdrqtail = 0;
	} else {
		hdrqtail = qib_get_rcvhdrtail(rcd);
		if (l == hdrqtail)
			goto bail;
		smp_rmb();  /* prevent speculative reads of dma'ed hdrq */
	}

	for (last = 0, i = 1; !last; i += !last) {
		hdr = dd->f_get_msgheader(dd, rhf_addr);
		eflags = rhf_err_flags(rhf_addr);
		etype = rhf_rcv_type(rhf_addr);
		/* total length */
		tlen = rhf_pkt_len(rhf_addr);	/* in bytes */
		ebuf = NULL;
		/* retreive eager buffer details */
		if (rhf_use_egr_bfr(rhf_addr)) {
			etail = rhf_egr_index(rhf_addr);
			updegr = 1;
			ebuf = qib_get_egrbuf(rcd, etail);
			/* TODO: Keep the prefetch?  Why are we doing it? */
			/*
			 * Prefetch the contents of the eager buffer.  It is
			 * OK to send a negative length to prefetch_range().
			 * The +2 is the size of the RHF.
			 */
			prefetch_range(ebuf,
				tlen - ((dd->rcvhdrentsize -
					  (rhf_hdrq_offset(rhf_addr)+2)) * 4));
		}
		trace_hfi_rcvhdr(dd,
				 rcd->ctxt,
				 eflags,
				 etype,
				 tlen,
				 updegr,
				 etail);
		/*
		 * TODO: This is a redundant check and should eventually
		 * be removed - the HW is doing the checking for us.
		 * Leave for now to double check the simulator.
		 */
		if (!eflags) {
			u16 lrh_len = be16_to_cpu(hdr->lrh[2]) << 2;

			if (lrh_len != tlen) {
				qib_stats.sps_lenerrs++;
				dd_dev_err(dd,
					"%s: tlen %d != lrh_len %d, skipping\n",
					__func__, tlen, (u32)lrh_len);
				goto move_along;
			}
		}
		/*
		 * TODO: What is the purpose of this check?  It looks like
		 * it is dropping an IB packet if didn't use an eager buffer
		 * but its total length is larger than 1 DWORD more than
		 * the header. (Why the +1?)
		 * This looks redundant - and should be removed.
		 */
		if (etype == RHF_RCV_TYPE_IB && !eflags &&
		    ebuf == NULL &&
		    tlen > (dd->rcvhdrentsize - 2 + 1 -
				rhf_hdrq_offset(rhf_addr)) << 2) {
			dd_dev_err(dd, "%s: IB, no errors, no eager, tlen > headerlen+1, skipping\n", __func__);
			goto move_along;
		}

		if (unlikely(eflags))
			rcv_hdrerr(rcd, ppd, rcd->ctxt, eflags, l,
					       etail, rhf_addr, hdr);
		else if (etype == RHF_RCV_TYPE_IB)
			qib_ib_rcv(rcd, hdr, ebuf, tlen);
		/* FIXME: Handle bypass packets */
move_along:
		l += rsize;
		if (l >= maxcnt)
			l = 0;
		if (i == MAX_PKT_RECV)
			last = 1;

		rhf_addr = (__le32 *) rcd->rcvhdrq + l + dd->rhf_offset;
		if (dd->flags & QIB_NODMA_RTAIL) {
			u32 seq = rhf_rcv_seq(rhf_addr);

			if (++rcd->seq_cnt > 13)
				rcd->seq_cnt = 1;
			if (seq != rcd->seq_cnt)
				last = 1;
		} else if (l == hdrqtail)
			last = 1;
		/*
		 * Update head regs etc., every 16 packets, if not last pkt,
		 * to help prevent rcvhdrq overflows, when many packets
		 * are processed and queue is nearly full.
		 * Don't request an interrupt for intermediate updates.
		 */
		lval = l;
		if (!last && !(i & 0xf)) {
			dd->f_update_usrhead(rcd, lval, updegr, etail, i);
			updegr = 0;
		}
	}
	/*
	 * Notify qib_destroy_qp() if it is waiting
	 * for lookaside_qp to finish.
	 */
	if (rcd->lookaside_qp) {
		if (atomic_dec_and_test(&rcd->lookaside_qp->refcount))
			wake_up(&rcd->lookaside_qp->wait);
		rcd->lookaside_qp = NULL;
	}

	rcd->head = l;
	rcd->pkt_count += i;

	/*
	 * Iterate over all QPs waiting to respond.
	 * The list won't change since the IRQ is only run on one CPU.
	 */
	list_for_each_entry_safe(qp, nqp, &rcd->qp_wait_list, rspwait) {
		list_del_init(&qp->rspwait);
		if (qp->r_flags & QIB_R_RSP_NAK) {
			qp->r_flags &= ~QIB_R_RSP_NAK;
			qib_send_rc_ack(qp);
		}
		if (qp->r_flags & QIB_R_RSP_SEND) {
			unsigned long flags;

			qp->r_flags &= ~QIB_R_RSP_SEND;
			spin_lock_irqsave(&qp->s_lock, flags);
			if (ib_qib_state_ops[qp->state] &
					QIB_PROCESS_OR_FLUSH_SEND)
				qib_schedule_send(qp);
			spin_unlock_irqrestore(&qp->s_lock, flags);
		}
		if (atomic_dec_and_test(&qp->refcount))
			wake_up(&qp->wait);
	}

bail:
	/*
	 * Always write head at end, and setup rcv interrupt, even
	 * if no packets were processed.
	 */
	lval = (u64)rcd->head | dd->rhdrhead_intr_off;
	dd->f_update_usrhead(rcd, lval, updegr, etail, i);
}

/*
 * Convert a given MTU size to the on-wire MAD packet enumeration.
 * Return -1 if the size is invalid.
 */
int mtu_to_enum(u32 mtu, int default_if_bad)
{
	switch (mtu) {
	case   256: return STL_MTU_256;
	case  1024: return STL_MTU_1024;
	case  2048: return STL_MTU_2048;
	case  4096: return STL_MTU_4096;
	case  8192: return STL_MTU_8192;
	case 10240: return STL_MTU_10240;
	}
	return default_if_bad;
}

/*
 * set_mtu - set the MTU
 * @ppd: the perport data
 * @arg: the new MTU
 *
 * We can handle "any" incoming size, the issue here is whether we
 * need to restrict our outgoing size.  We do not deal with what happens
 * to programs that are already running when the size changes.
 */
int set_mtu(struct qib_pportdata *ppd, u16 arg)
{
	if (!valid_mtu(arg))
		return -EINVAL;
	if (arg > max_mtu)
		return -EINVAL;

	ppd->ibmtu = arg;
	ppd->ibmaxlen = arg + lrh_max_header_bytes(ppd->dd);
	ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_MTU, arg);

	return 0;
}

int qib_set_lid(struct qib_pportdata *ppd, u32 lid, u8 lmc)
{
	struct hfi_devdata *dd = ppd->dd;

	ppd->lid = lid;
	ppd->lmc = lmc;
	dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LIDLMC, 0);

	dd_dev_info(dd, "IB%u:%u got a lid: 0x%x\n", dd->unit, ppd->port, lid);

	return 0;
}

/*
 * Following deal with the "obviously simple" task of overriding the state
 * of the LEDS, which normally indicate link physical and logical status.
 * The complications arise in dealing with different hardware mappings
 * and the board-dependent routine being called from interrupts.
 * and then there's the requirement to _flash_ them.
 */
#define LED_OVER_FREQ_SHIFT 8
#define LED_OVER_FREQ_MASK (0xFF<<LED_OVER_FREQ_SHIFT)
/* Below is "non-zero" to force override, but both actual LEDs are off */
#define LED_OVER_BOTH_OFF (8)

static void qib_run_led_override(unsigned long opaque)
{
	struct qib_pportdata *ppd = (struct qib_pportdata *)opaque;
	struct hfi_devdata *dd = ppd->dd;
	int timeoff;
	int ph_idx;

	if (!(dd->flags & QIB_INITTED))
		return;

	ph_idx = ppd->led_override_phase++ & 1;
	ppd->led_override = ppd->led_override_vals[ph_idx];
	timeoff = ppd->led_override_timeoff;

	dd->f_setextled(ppd, 1);
	/*
	 * don't re-fire the timer if user asked for it to be off; we let
	 * it fire one more time after they turn it off to simplify
	 */
	if (ppd->led_override_vals[0] || ppd->led_override_vals[1])
		mod_timer(&ppd->led_override_timer, jiffies + timeoff);
}

void qib_set_led_override(struct qib_pportdata *ppd, unsigned int val)
{
	struct hfi_devdata *dd = ppd->dd;
	int timeoff, freq;

	if (!(dd->flags & QIB_INITTED))
		return;

	/* First check if we are blinking. If not, use 1HZ polling */
	timeoff = HZ;
	freq = (val & LED_OVER_FREQ_MASK) >> LED_OVER_FREQ_SHIFT;

	if (freq) {
		/* For blink, set each phase from one nybble of val */
		ppd->led_override_vals[0] = val & 0xF;
		ppd->led_override_vals[1] = (val >> 4) & 0xF;
		timeoff = (HZ << 4)/freq;
	} else {
		/* Non-blink set both phases the same. */
		ppd->led_override_vals[0] = val & 0xF;
		ppd->led_override_vals[1] = val & 0xF;
	}
	ppd->led_override_timeoff = timeoff;

	/*
	 * If the timer has not already been started, do so. Use a "quick"
	 * timeout so the function will be called soon, to look at our request.
	 */
	if (atomic_inc_return(&ppd->led_override_timer_active) == 1) {
		/* Need to start timer */
		init_timer(&ppd->led_override_timer);
		ppd->led_override_timer.function = qib_run_led_override;
		ppd->led_override_timer.data = (unsigned long) ppd;
		ppd->led_override_timer.expires = jiffies + 1;
		add_timer(&ppd->led_override_timer);
	} else {
		if (ppd->led_override_vals[0] || ppd->led_override_vals[1])
			mod_timer(&ppd->led_override_timer, jiffies + 1);
		atomic_dec(&ppd->led_override_timer_active);
	}
}

/**
 * qib_reset_device - reset the chip if possible
 * @unit: the device to reset
 *
 * Whether or not reset is successful, we attempt to re-initialize the chip
 * (that is, much like a driver unload/reload).  We clear the INITTED flag
 * so that the various entry points will fail until we reinitialize.  For
 * now, we only allow this if no user contexts are open that use chip resources
 */
int qib_reset_device(int unit)
{
	int ret, i;
	struct hfi_devdata *dd = qib_lookup(unit);
	struct qib_pportdata *ppd;
	unsigned long flags;
	int pidx;

	if (!dd) {
		ret = -ENODEV;
		goto bail;
	}

	dd_dev_info(dd, "Reset on unit %u requested\n", unit);

	if (!dd->kregbase || !(dd->flags & QIB_PRESENT)) {
		dd_dev_info(dd,
			"Invalid unit number %u or not initialized or not present\n",
			unit);
		ret = -ENXIO;
		goto bail;
	}

	spin_lock_irqsave(&dd->uctxt_lock, flags);
	if (dd->rcd)
		for (i = dd->first_user_ctxt; i < dd->num_rcv_contexts; i++) {
			if (!dd->rcd[i] || !dd->rcd[i]->cnt)
				continue;
			spin_unlock_irqrestore(&dd->uctxt_lock, flags);
			ret = -EBUSY;
			goto bail;
		}
	spin_unlock_irqrestore(&dd->uctxt_lock, flags);

	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;
		if (atomic_read(&ppd->led_override_timer_active)) {
			/* Need to stop LED timer, _then_ shut off LEDs */
			del_timer_sync(&ppd->led_override_timer);
			atomic_set(&ppd->led_override_timer_active, 0);
		}

		/* Shut off LEDs after we are sure timer is not running */
		ppd->led_override = LED_OVER_BOTH_OFF;
		dd->f_setextled(ppd, 0);
		if (dd->flags & QIB_HAS_SEND_DMA)
			qib_teardown_sdma(ppd);
	}

	ret = dd->f_reset(dd);
	if (ret == 1)
		ret = qib_init(dd, 1);
	else
		ret = -EAGAIN;
	if (ret)
		dd_dev_err(dd,
			"Reinitialize unit %u after reset failed with %d\n",
			unit, ret);
	else
		dd_dev_info(dd, "Reinitialized unit %u after resetting\n",
			unit);

bail:
	return ret;
}
