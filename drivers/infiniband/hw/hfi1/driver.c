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
#include "sdma.h"

#undef pr_fmt
#define pr_fmt(fmt) DRIVER_NAME ": " fmt

/*
 * The size has to be longer than this string, so we can append
 * board/chip information to it in the init code.
 */
const char ib_hfi1_version[] = HFI_DRIVER_VERSION "\n";

DEFINE_SPINLOCK(hfi1_devs_lock);
LIST_HEAD(hfi1_dev_list);
DEFINE_MUTEX(hfi1_mutex);	/* general driver use */

unsigned int max_mtu;
module_param_named(max_mtu, max_mtu, uint, S_IRUGO);
MODULE_PARM_DESC(max_mtu, "Set max MTU bytes, default is 8192");

unsigned int default_mtu;
module_param_named(default_mtu, default_mtu, uint, S_IRUGO);
MODULE_PARM_DESC(default_mtu, "Set default MTU bytes, default is 4096");

unsigned int hfi_cu = 1;
module_param_named(cu, hfi_cu, uint, S_IRUGO);
MODULE_PARM_DESC(cu, "Credit return units");

unsigned long hfi_cap_mask = HFI_CAP_MASK_DEFAULT;
static int hfi_caps_set(const char *, const struct kernel_param *);
static int hfi_caps_get(char *, const struct kernel_param *);
static const struct kernel_param_ops cap_ops = {
	.set = hfi_caps_set,
	.get = hfi_caps_get
};
module_param_cb(cap_mask, &cap_ops, &hfi_cap_mask, S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(cap_mask, "Bit mask of enabled/disabled HW features");

/*
 * Start parameter backward compatibility code (will be removed)
 *
 * The definitions below add module parameters which get converted
 * into the appropriate bits in hfi_cap_mask. These module parameters
 * will eventually be removed once module users have become
 * familiar with the hfi_cap_mask parameter.
 */
#ifndef HFI_COMPAT_MODPARAMS
#define HFI_COMPAT_MODPARAMS 1
#endif

#if HFI_COMPAT_MODPARAMS
#include <linux/stringify.h>
struct hfi_comp_param_t {
	const char *pname;
	unsigned long equiv;
	u8 inverted;
	unsigned long *caps;
};
static int hfi_comp_param_set(const char *, const struct kernel_param *);
static int hfi_comp_param_get(char *, const struct kernel_param *);
static const struct kernel_param_ops k_pops_comp = {
	.set = hfi_comp_param_set,
	.get = hfi_comp_param_get
};
/**
 * HFI_DEFINE_COMP_PARAM - define a backward-compatible module parameter
 * @name - name of the module parameter
 * @bit  - the bit in the set of HFI_CAP_* bits that this module parameter
 *         corresponds to
 * @invrt - 1 if the value of this module parameter is the inverted value
 *          of the bit - value of 1 turns the bit off
 * @desc - the description string for this module parameter
 *
 * This macro defines a backward-compatible module parameter, which can
 * be used instead of the matching bit in the HFI_CAP_* set.
 * Modifying/using the module parameter will manipulate the hfi_cap_mask
 * bitmask. Therefore, the module parameter does not hold a value - the
 * hfi_cap_mask bits should be used through-out the code.
 */
#define HFI_DEFINE_COMP_PARAM(name, bit, invrt, desc)			\
	static const struct hfi_comp_param_t c_param_##name = {		\
		.pname = __stringify(name), .equiv = HFI_CAP_##bit,	\
		.inverted = (unsigned)invrt, .caps = &hfi_cap_mask,	\
	};								\
	module_param_cb(name, &k_pops_comp, (void *)&c_param_##name,	\
			(S_IRUGO |					\
			 (HFI_CAP_##bit & HFI_CAP_WRITABLE_MASK ?	\
			  S_IWUSR : 0)));				\
	MODULE_PARM_DESC(name, desc " (DEPRECATED)")

/* Define all the depricated module parameters */
HFI_DEFINE_COMP_PARAM(hdrsup_enable, HDRSUPP, 0,
		      "Enable/disable header suppression");
HFI_DEFINE_COMP_PARAM(dont_drop_rhq_full, NODROP_RHQ_FULL, 0,
		      "Do not drop packets when the receive header is full");
HFI_DEFINE_COMP_PARAM(dont_drop_egr_full, NODROP_EGR_FULL, 0,
		      "Do not drop packets when all eager buffers are in use");
HFI_DEFINE_COMP_PARAM(use_sdma_head, USE_SDMA_HEAD, 1,
		      "Read CSR vs. DMA for hardware head");
HFI_DEFINE_COMP_PARAM(use_sdma_ahg, SDMA_AHG, 0, "Turn on/off use of AHG");
HFI_DEFINE_COMP_PARAM(disable_sma, ENABLE_SMA, 1, "Disable the SMA");
HFI_DEFINE_COMP_PARAM(nodma_rtail, DMA_RTAIL, 1,
		      "1 for no DMA of hdr tail, 0 to DMA the hdr tail");
HFI_DEFINE_COMP_PARAM(use_sdma, SDMA, 0, "enable sdma traffic");
HFI_DEFINE_COMP_PARAM(extended_psn, EXTENDED_PSN, 0,
		      "Use 24 or 31 bit PSN");
HFI_DEFINE_COMP_PARAM(print_unimplemented, PRINT_UNIMPL, 0,
		      "Have unimplemented functions print when called");
HFI_DEFINE_COMP_PARAM(one_pkt_per_egr, MULTI_PKT_EGR, 1,
		      "Use one packet per eager buffer (default: 0)");
HFI_DEFINE_COMP_PARAM(enable_pkeys, PKEY_CHECK, 0,
		      "Enable PKey checking on receive");
HFI_DEFINE_COMP_PARAM(disable_integrity, NO_INTEGRITY, 0,
		      "Disablep HW packet integrity checks");
HFI_DEFINE_COMP_PARAM(sdma_head_check, SDMA_HEAD_CHECK, 0,
		      "Enable SDMA head check");
#endif /* HFI_COMPAT_MODPARAMS */
/* End parameter backward compatibility code */

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel <ibsupport@intel.com>");
MODULE_DESCRIPTION("Intel IB driver");
MODULE_VERSION(HFI_DRIVER_VERSION);

/* See hfi1_init() */
void (*rhf_rcv_function_map[5])(struct hfi_packet *packet);

/*
 * MAX_PKT_RCV is the max # if packets processed per receive interrupt.
 */
#define MAX_PKT_RECV 64
#define EGR_HEAD_UPDATE_THRESHOLD 16

struct hfi1_ib_stats hfi1_stats;

static int hfi_caps_set(const char *val, const struct kernel_param *kp)
{
	int ret = 0;
	unsigned long *cap_mask_ptr = (unsigned long *)kp->arg,
		cap_mask = *cap_mask_ptr, value, diff,
		write_mask = ((HFI_CAP_WRITABLE_MASK << HFI_CAP_USER_SHIFT) |
			      HFI_CAP_WRITABLE_MASK);

	ret = kstrtoul(val, 0, &value);
	if (ret) {
		pr_warn("Invalid module parameter value for 'cap_mask'\n");
		goto done;
	}
	/* Get the changed bits (except the locked bit) */
	diff = value ^ (cap_mask & ~HFI_CAP_LOCKED_SMASK);

	/* Remove any bits that are not allowed to change after driver load */
	if (HFI_CAP_LOCKED() && (diff & ~write_mask)) {
		pr_warn("Ignoring non-writable capability bits %#lx\n",
			diff & ~write_mask);
		diff &= write_mask;
	}

	/* Mask off any reserved bits */
	diff &= ~HFI_CAP_RESERVED_MASK;
	/* Clear any previously set and changing bits */
	cap_mask &= ~diff;
	/* Update the bits with the new capability */
	cap_mask |= (value & diff);
	/* Check for any kernel/user restrictions */
	diff = (cap_mask & (HFI_CAP_MUST_HAVE_KERN << HFI_CAP_USER_SHIFT)) ^
		((cap_mask & HFI_CAP_MUST_HAVE_KERN) << HFI_CAP_USER_SHIFT);
	cap_mask &= ~diff;
	/* Set the bitmask to the final set */
	*cap_mask_ptr = cap_mask;
done:
	return ret;
}

static int hfi_caps_get(char *buffer, const struct kernel_param *kp)
{
	unsigned long cap_mask = *(unsigned long *)kp->arg;

	cap_mask &= ~HFI_CAP_LOCKED_SMASK;
	cap_mask |= ((cap_mask & HFI_CAP_K2U) << HFI_CAP_USER_SHIFT);

	return scnprintf(buffer, PAGE_SIZE, "0x%lx", cap_mask);
}

#if HFI_COMPAT_MODPARAMS
static int hfi_comp_param_set(const char *val, const struct kernel_param *kp)
{
	struct hfi_comp_param_t *param =
		(struct hfi_comp_param_t *)kp->arg;
	unsigned long value;
	int ret = 0;

	pr_warn("Module parameter '%s' is depricated in favor of 'cap_mask'\n",
		param->pname);
	ret = kstrtoul(val, 0, &value);
	if (ret) {
		pr_warn("Invalid parameter value '%s'\n", val);
		goto done;
	}

	value = (value & 0x1U) ^ param->inverted;
	if (value)
		*param->caps |= (param->equiv |
				 ((param->equiv & HFI_CAP_WRITABLE_MASK) <<
				  HFI_CAP_USER_SHIFT));
	else
		*param->caps &= ~(param->equiv |
				  ((param->equiv & HFI_CAP_WRITABLE_MASK) <<
				   HFI_CAP_USER_SHIFT));
done:
	return ret;
}

static int hfi_comp_param_get(char *buffer, const struct kernel_param *kp)
{
	struct hfi_comp_param_t *param =
		(struct hfi_comp_param_t *)kp->arg;
	unsigned long value = *param->caps & param->equiv;

	return scnprintf(buffer, PAGE_SIZE, "%d",
			 !!(param->inverted ? !value : value));
}
#endif

const char *get_unit_name(int unit)
{
	static char iname[16];

	snprintf(iname, sizeof(iname), DRIVER_NAME"_%u", unit);
	return iname;
}

/*
 * Return count of units with at least one port ACTIVE.
 */
int hfi1_count_active_units(void)
{
	struct hfi_devdata *dd;
	struct hfi1_pportdata *ppd;
	unsigned long flags;
	int pidx, nunits_active = 0;

	spin_lock_irqsave(&hfi1_devs_lock, flags);
	list_for_each_entry(dd, &hfi1_dev_list, list) {
		if (!(dd->flags & HFI_PRESENT) || !dd->kregbase)
			continue;
		for (pidx = 0; pidx < dd->num_pports; ++pidx) {
			ppd = dd->pport + pidx;
			if (ppd->lid && ppd->linkup) {
				nunits_active++;
				break;
			}
		}
	}
	spin_unlock_irqrestore(&hfi1_devs_lock, flags);
	return nunits_active;
}

/*
 * Return count of all units, optionally return in arguments
 * the number of usable (present) units, and the number of
 * ports that are up.
 */
int hfi1_count_units(int *npresentp, int *nupp)
{
	int nunits = 0, npresent = 0, nup = 0;
	struct hfi_devdata *dd;
	unsigned long flags;
	int pidx;
	struct hfi1_pportdata *ppd;

	spin_lock_irqsave(&hfi1_devs_lock, flags);

	list_for_each_entry(dd, &hfi1_dev_list, list) {
		nunits++;
		if ((dd->flags & HFI_PRESENT) && dd->kregbase)
			npresent++;
		for (pidx = 0; pidx < dd->num_pports; ++pidx) {
			ppd = dd->pport + pidx;
			if (ppd->lid && ppd->linkup)
				nup++;
		}
	}

	spin_unlock_irqrestore(&hfi1_devs_lock, flags);

	if (npresentp)
		*npresentp = npresent;
	if (nupp)
		*nupp = nup;

	return nunits;
}

/*
 * Get address of eager buffer from it's index (allocated in chunks, not
 * contiguous).
 */
static inline void *get_egrbuf(const struct hfi1_ctxtdata *rcd, u64 rhf,
			       u32 *update)
{
	u32 idx = rhf_egr_index(rhf), offset = rhf_egr_buf_offset(rhf);

	*update |= !(idx & (rcd->egrbufs.threshold - 1)) && !offset;
	return (void *)(((u64)(rcd->egrbufs.rcvtids[idx].addr)) +
			(offset * RCV_BUF_BLOCK_SIZE));
}

/*
 * Validate and encode the a given RcvArray Buffer size.
 * The fuction will check whether the given size falls within
 * allowed size ranges for the respective type and, optionally,
 * return the proper encoding.
 */
inline int hfi_rcvbuf_validate(u32 size, u8 type, u16 *encoded)
{
	if (unlikely(!IS_ALIGNED(size, PAGE_SIZE)))
		return 0;
	if (unlikely(size < MIN_EAGER_BUFFER))
		return 0;
	if (size >
	    (type == PT_EAGER ? MAX_EAGER_BUFFER : MAX_EXPECTED_BUFFER))
		return 0;
	if (encoded)
		*encoded = ilog2(size / PAGE_SIZE) + 1;
	return 1;
}

static void rcv_hdrerr(struct hfi1_ctxtdata *rcd, struct hfi1_pportdata *ppd,
		       struct hfi_packet *packet)
{
	struct hfi1_message_header *rhdr = packet->hdr;
	u32 rte = rhf_rcv_type_err(packet->rhf);
	int lnh = be16_to_cpu(rhdr->lrh[0]) & 3;
	struct hfi1_ibport *ibp = &ppd->ibport_data;

	if (packet->rhf & (RHF_VCRC_ERR | RHF_ICRC_ERR))
		return;

	if (packet->rhf & RHF_TID_ERR) {
		/* For TIDERR and RC QPs premptively schedule a NAK */
		struct hfi1_ib_header *hdr = (struct hfi1_ib_header *)rhdr;
		struct hfi1_other_headers *ohdr = NULL;
		u32 tlen = rhf_pkt_len(packet->rhf); /* in bytes */
		u16 lid  = be16_to_cpu(hdr->lrh[1]);
		u32 qp_num;
		u32 rcv_flags = 0;

		/* Sanity check packet */
		if (tlen < 24)
			goto drop;

		/* Check for GRH */
		if (lnh == HFI1_LRH_BTH)
			ohdr = &hdr->u.oth;
		else if (lnh == HFI1_LRH_GRH) {
			u32 vtf;

			ohdr = &hdr->u.l.oth;
			if (hdr->u.l.grh.next_hdr != IB_GRH_NEXT_HDR)
				goto drop;
			vtf = be32_to_cpu(hdr->u.l.grh.version_tclass_flow);
			if ((vtf >> IB_GRH_VERSION_SHIFT) != IB_GRH_VERSION)
				goto drop;
			rcv_flags |= HFI1_HAS_GRH;
		} else
			goto drop;

		/* Get the destination QP number. */
		qp_num = be32_to_cpu(ohdr->bth[1]) & HFI1_QPN_MASK;
		if (lid < HFI1_MULTICAST_LID_BASE) {
			struct hfi1_qp *qp;

			qp = hfi1_lookup_qpn(ibp, qp_num);
			if (!qp)
				goto drop;

			/*
			 * Handle only RC QPs - for other QP types drop error
			 * packet.
			 */
			spin_lock(&qp->r_lock);

			/* Check for valid receive state. */
			if (!(ib_hfi1_state_ops[qp->state] &
			      HFI1_PROCESS_RECV_OK)) {
				ibp->n_pkt_drops++;
			}

			switch (qp->ibqp.qp_type) {
			case IB_QPT_RC:
				hfi1_rc_hdrerr(
					rcd,
					hdr,
					rcv_flags,
					qp);
				break;
			default:
				/* For now don't handle any other QP types */
				break;
			}

			spin_unlock(&qp->r_lock);
			/*
			 * Notify hfi1_destroy_qp() if it is waiting
			 * for us to finish.
			 */
			if (atomic_dec_and_test(&qp->refcount))
				wake_up(&qp->wait);
		} /* Unicast QP */
	} /* Valid packet with TIDErr */

	/* handle "RcvTypeErr" flags */
	switch (rte) {
	case RHF_RTE_ERROR_OP_CODE_ERR:
	{
		u32 opcode;
		void *ebuf = NULL;
		__be32 *bth = NULL;

		if (rhf_use_egr_bfr(packet->rhf))
			ebuf = packet->ebuf;

		if (ebuf == NULL)
			goto drop; /* this should never happen */

		if (lnh == HFI1_LRH_BTH)
			bth = (__be32 *)ebuf;
		else if (lnh == HFI1_LRH_GRH)
			bth = (__be32 *)((char *)ebuf + sizeof(struct ib_grh));
		else
			goto drop;

		opcode = be32_to_cpu(bth[0]) >> 24;
		opcode &= 0xff;

		if (opcode == CNP_OPCODE) {
			/*
			 * Only in pre-B0 h/w is the CNP_OPCODE handled
			 * via this code path (errata 291394).
			 */
			struct hfi1_qp *qp = NULL;
			u32 lqpn, rqpn;
			u16 rlid;
			u8 svc_type, sl, sc5;

			sc5  = be16_to_cpu(rhdr->lrh[0] >> 4) & 0xf;
			if (rhf_dc_info(packet->rhf))
				sc5 |= 0x10;
			sl = ibp->sc_to_sl[sc5];

			lqpn = be32_to_cpu(bth[1]) & HFI1_QPN_MASK;
			qp = hfi1_lookup_qpn(ibp, lqpn);
			if (qp == NULL)
				goto drop;

			switch (qp->ibqp.qp_type) {
			case IB_QPT_UD:
				rlid = 0;
				rqpn = 0;
				svc_type = IB_CC_SVCTYPE_UD;
				break;
			case IB_QPT_UC:
				rlid = be16_to_cpu(rhdr->lrh[3]);
				rqpn = qp->remote_qpn;
				svc_type = IB_CC_SVCTYPE_UC;
				break;
			default:
				goto drop;
			}
			/* drop qp->refcount, wake waiters if it's 0 */
			if (atomic_dec_and_test(&qp->refcount))
				wake_up(&qp->wait);

			process_becn(ppd, sl, rlid, lqpn, rqpn, svc_type);
		}

		packet->rhf &= ~RHF_RCV_TYPE_ERR_SMASK;
		break;
	}
	default:
		break;
	}

drop:
	return;
}

/*
 * handle_receive_interrupt - receive a packet
 * @rcd: the context
 *
 * Called from interrupt handler for errors or receive interrupt.
 */
void handle_receive_interrupt(struct hfi1_ctxtdata *rcd)
{
	struct hfi_devdata *dd = rcd->dd;
	__le32 *rhf_addr;
	u64 rhf;
	void *ebuf;
	const u32 rsize = rcd->rcvhdrqentsize;        /* words */
	const u32 maxcnt = rcd->rcvhdrq_cnt * rsize;   /* words */
	u32 etail = -1, l, hdrqtail;
	struct hfi1_message_header *hdr;
	u32 etype, hlen, tlen, i = 0, updegr = 0;
	int last;
	struct hfi1_qp *qp, *nqp;
	struct hfi_packet packet;

	l = rcd->head;
	rhf_addr = (__le32 *) rcd->rcvhdrq + l + dd->rhf_offset;
	rhf = rhf_to_cpu(rhf_addr);

	if (!HFI_CAP_IS_KSET(DMA_RTAIL)) {
		u32 seq = rhf_rcv_seq(rhf);

		if (seq != rcd->seq_cnt)
			goto bail;
		hdrqtail = 0;
	} else {
		hdrqtail = get_rcvhdrtail(rcd);
		if (l == hdrqtail)
			goto bail;
		smp_rmb();  /* prevent speculative reads of dma'ed hdrq */
	}

	for (last = 0, i = 1; !last; i += !last) {
		hdr = hfi1_get_msgheader(dd, rhf_addr);
		hlen = (u8 *)rhf_addr - (u8 *)hdr;
		etype = rhf_rcv_type(rhf);
		/* total length */
		tlen = rhf_pkt_len(rhf);	/* in bytes */
		ebuf = NULL;
		/* retreive eager buffer details */
		if (rhf_use_egr_bfr(rhf)) {
			etail = rhf_egr_index(rhf);
			ebuf = get_egrbuf(rcd, rhf, &updegr);
			/*
			 * Prefetch the contents of the eager buffer.  It is
			 * OK to send a negative length to prefetch_range().
			 * The +2 is the size of the RHF.
			 */
			prefetch_range(ebuf,
				tlen - ((rcd->rcvhdrqentsize -
					  (rhf_hdrq_offset(rhf)+2)) * 4));
		}

		/*
		 * A0 erratum 291500: Drop the first packet in HFI. It is
		 * corrupted after power-on reset.
		 */
		if (unlikely(dd->do_drop && atomic_xchg(&dd->drop_packet,
			DROP_PACKET_OFF) == DROP_PACKET_ON)) {
			dd->do_drop = 0;
			goto skip;
		}

		packet.tlen = tlen;
		packet.hlen = hlen;
		packet.rhf = rhf;
		packet.ebuf = ebuf;
		packet.hdr = hdr;
		packet.rcd = rcd;
		packet.updegr = updegr;

		/*
		 * Call a type specific handler for the packet. We
		 * should be able to trust that etype won't be beyond
		 * the range of valid indexes. If so something is really
		 * wrong and we can probably just let things come
		 * crashing down. There is no need to eat another
		 * comparision in this performance critical code.
		 */
		rhf_rcv_function_map[etype](&packet);

		/* On to the next packet */
skip:
		l += rsize;
		if (l >= maxcnt)
			l = 0;
		if (i == MAX_PKT_RECV)
			last = 1;

		rhf_addr = (__le32 *) rcd->rcvhdrq + l + dd->rhf_offset;
		rhf = rhf_to_cpu(rhf_addr);

		if (!HFI_CAP_IS_KSET(DMA_RTAIL)) {
			u32 seq = rhf_rcv_seq(rhf);

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
		if (!last && !(i & 0xf)) {
			update_usrhead(rcd, l, updegr, etail, 0, 0);
			updegr = 0;
		}
	}
	/*
	 * Notify hfi1_destroy_qp() if it is waiting
	 * for lookaside_qp to finish.
	 */
	if (rcd->lookaside_qp) {
		if (atomic_dec_and_test(&rcd->lookaside_qp->refcount))
			wake_up(&rcd->lookaside_qp->wait);
		rcd->lookaside_qp = NULL;
	}

	rcd->head = l;

	/*
	 * Iterate over all QPs waiting to respond.
	 * The list won't change since the IRQ is only run on one CPU.
	 */
	list_for_each_entry_safe(qp, nqp, &rcd->qp_wait_list, rspwait) {
		list_del_init(&qp->rspwait);
		if (qp->r_flags & HFI1_R_RSP_NAK) {
			qp->r_flags &= ~HFI1_R_RSP_NAK;
			hfi1_send_rc_ack(rcd, qp, 0);
		}
		if (qp->r_flags & HFI1_R_RSP_SEND) {
			unsigned long flags;

			qp->r_flags &= ~HFI1_R_RSP_SEND;
			spin_lock_irqsave(&qp->s_lock, flags);
			if (ib_hfi1_state_ops[qp->state] &
					HFI1_PROCESS_OR_FLUSH_SEND)
				hfi1_schedule_send(qp);
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
	update_usrhead(rcd, rcd->head, updegr, etail, rcv_intr_dynamic, i);
}

/*
 * Convert a given MTU size to the on-wire MAD packet enumeration.
 * Return -1 if the size is invalid.
 */
int mtu_to_enum(u32 mtu, int default_if_bad)
{
	switch (mtu) {
	case     0: return OPA_MTU_0;
	case   256: return OPA_MTU_256;
	case   512: return OPA_MTU_512;
	case  1024: return OPA_MTU_1024;
	case  2048: return OPA_MTU_2048;
	case  4096: return OPA_MTU_4096;
	case  8192: return OPA_MTU_8192;
	case 10240: return OPA_MTU_10240;
	}
	return default_if_bad;
}

u16 enum_to_mtu(int mtu)
{
	switch (mtu) {
	case OPA_MTU_0:     return 0;
	case OPA_MTU_256:   return 256;
	case OPA_MTU_512:   return 512;
	case OPA_MTU_1024:  return 1024;
	case OPA_MTU_2048:  return 2048;
	case OPA_MTU_4096:  return 4096;
	case OPA_MTU_8192:  return 8192;
	case OPA_MTU_10240: return 10240;
	default: return 0xffff;
	}
}

/*
 * set_mtu - set the MTU
 * @ppd: the per port data
 *
 * We can handle "any" incoming size, the issue here is whether we
 * need to restrict our outgoing size.  We do not deal with what happens
 * to programs that are already running when the size changes.
 */
int set_mtu(struct hfi1_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;
	int i, drain, ret = 0, is_up = 0;

	ppd->ibmtu = 0;
	for (i = 0; i < hfi_num_vls(ppd->vls_supported); i++)
		if (ppd->ibmtu < dd->vld[i].mtu)
			ppd->ibmtu = dd->vld[i].mtu;
	ppd->ibmaxlen = ppd->ibmtu + lrh_max_header_bytes(ppd->dd);

	mutex_lock(&ppd->hls_lock);
	if (ppd->host_link_state == HLS_UP_INIT
			|| ppd->host_link_state == HLS_UP_ARMED
			|| ppd->host_link_state == HLS_UP_ACTIVE)
		is_up = 1;

	drain = !is_ax(dd) && is_up;

	if (drain)
		/*
		 * MTU is specified per-VL. To ensure that no packet gets
		 * stuck (due, e.g., to the MTU for the packet's VL being
		 * reduced), empty the per-VL FIFOs before adjusting MTU.
		 */
		ret = stop_drain_data_vls(dd);

	if (ret) {
		dd_dev_err(dd, "%s: cannot stop/drain VLs - refusing to change per-VL MTUs\n",
			   __func__);
		goto err;
	}

	hfi1_set_ib_cfg(ppd, HFI1_IB_CFG_MTU, 0);

	if (drain)
		open_fill_data_vls(dd); /* reopen all VLs */

err:
	mutex_unlock(&ppd->hls_lock);

	return ret;
}

int hfi1_set_lid(struct hfi1_pportdata *ppd, u32 lid, u8 lmc)
{
	struct hfi_devdata *dd = ppd->dd;

	ppd->lid = lid;
	ppd->lmc = lmc;
	hfi1_set_ib_cfg(ppd, HFI1_IB_CFG_LIDLMC, 0);

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

static void run_led_override(unsigned long opaque)
{
	struct hfi1_pportdata *ppd = (struct hfi1_pportdata *)opaque;
	struct hfi_devdata *dd = ppd->dd;
	int timeoff;
	int ph_idx;

	if (!(dd->flags & HFI_INITTED))
		return;

	ph_idx = ppd->led_override_phase++ & 1;
	ppd->led_override = ppd->led_override_vals[ph_idx];
	timeoff = ppd->led_override_timeoff;

	/*
	 * don't re-fire the timer if user asked for it to be off; we let
	 * it fire one more time after they turn it off to simplify
	 */
	if (ppd->led_override_vals[0] || ppd->led_override_vals[1])
		mod_timer(&ppd->led_override_timer, jiffies + timeoff);
}

void hfi1_set_led_override(struct hfi1_pportdata *ppd, unsigned int val)
{
	struct hfi_devdata *dd = ppd->dd;
	int timeoff, freq;

	if (!(dd->flags & HFI_INITTED))
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
		ppd->led_override_timer.function = run_led_override;
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
 * hfi1_reset_device - reset the chip if possible
 * @unit: the device to reset
 *
 * Whether or not reset is successful, we attempt to re-initialize the chip
 * (that is, much like a driver unload/reload).  We clear the INITTED flag
 * so that the various entry points will fail until we reinitialize.  For
 * now, we only allow this if no user contexts are open that use chip resources
 */
int hfi1_reset_device(int unit)
{
	int ret, i;
	struct hfi_devdata *dd = hfi1_lookup(unit);
	struct hfi1_pportdata *ppd;
	unsigned long flags;
	int pidx;

	if (!dd) {
		ret = -ENODEV;
		goto bail;
	}

	dd_dev_info(dd, "Reset on unit %u requested\n", unit);

	if (!dd->kregbase || !(dd->flags & HFI_PRESENT)) {
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
	}
	if (dd->flags & HFI_HAS_SEND_DMA)
		sdma_exit(dd);

	hfi1_reset_cpu_counters(dd);

	ret = hfi1_init(dd, 1);

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

void handle_eflags(struct hfi_packet *packet)
{
	struct hfi1_ctxtdata *rcd = packet->rcd;
	u32 rte = rhf_rcv_type_err(packet->rhf);

	dd_dev_err(rcd->dd,
		"receive context %d: rhf 0x%016llx, errs [ %s%s%s%s%s%s%s%s] rte 0x%x\n",
		rcd->ctxt, packet->rhf,
		packet->rhf & RHF_K_HDR_LEN_ERR ? "k_hdr_len " : "",
		packet->rhf & RHF_DC_UNC_ERR ? "dc_unc " : "",
		packet->rhf & RHF_DC_ERR ? "dc " : "",
		packet->rhf & RHF_TID_ERR ? "tid " : "",
		packet->rhf & RHF_LEN_ERR ? "len " : "",
		packet->rhf & RHF_ECC_ERR ? "ecc " : "",
		packet->rhf & RHF_VCRC_ERR ? "vcrc " : "",
		packet->rhf & RHF_ICRC_ERR ? "icrc " : "",
		rte);

	rcv_hdrerr(rcd, rcd->ppd, packet);
}

/*
 * The following functions are called by the interrupt handler. They are type
 * specific handlers for each packet type.
 */
void process_receive_ib(struct hfi_packet *packet)
{
	trace_hfi_rcvhdr(packet->rcd->ppd->dd,
			 packet->rcd->ctxt,
			 rhf_err_flags(packet->rhf),
			 RHF_RCV_TYPE_IB,
			 packet->hlen,
			 packet->tlen,
			 packet->updegr,
			 rhf_egr_index(packet->rhf));

	if (unlikely(rhf_err_flags(packet->rhf))) {
		handle_eflags(packet);
		return;
	}

	hfi1_ib_rcv(packet);
}

void process_receive_bypass(struct hfi_packet *packet)
{
	if (unlikely(rhf_err_flags(packet->rhf)))
		handle_eflags(packet);

	dd_dev_err(packet->rcd->dd,
	   "Bypass packets are not supported in normal operation. Dropping\n");
}

void process_receive_error(struct hfi_packet *packet)
{
	handle_eflags(packet);
	dd_dev_err(packet->rcd->dd,
		   "Unhandled error packet received. Dropping.\n");
}

void process_receive_expected(struct hfi_packet *packet)
{
	if (unlikely(rhf_err_flags(packet->rhf)))
		handle_eflags(packet);

	dd_dev_err(packet->rcd->dd,
		   "Unhandled expected packet received. Dropping.\n");
}

void process_receive_eager(struct hfi_packet *packet)
{
	if (unlikely(rhf_err_flags(packet->rhf)))
		handle_eflags(packet);

	dd_dev_err(packet->rcd->dd,
		   "Unhandled eager packet received. Dropping.\n");
}

