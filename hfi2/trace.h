/*
 * Copyright(c) 2015, 2016 Intel Corporation.
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
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

/*
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */

#if !defined(__HFI_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define __HFI_TRACE_H

#undef TRACE_SYSTEM_VAR
#define TRACE_SYSTEM_VAR hfi2

#include <linux/tracepoint.h>
#include <linux/trace_seq.h>
#include <rdma/ib_verbs.h>

#include "opa_hfi.h"
#include "verbs/verbs.h"
#include "verbs/mad.h"
#include "verbs/packet.h"

#define DD_DEV_ENTRY(dd)       __string(dev, dev_name(&(dd)->pcidev->dev))
#define DD_DEV_ASSIGN(dd)      __assign_str(dev, dev_name(&(dd)->pcidev->dev))

#define packettype_name(etype) { RHF_RCV_TYPE_##etype, #etype }
#define show_packettype(etype)                  \
__print_symbolic(etype,                         \
	packettype_name(EXPECTED),              \
	packettype_name(EAGER),                 \
	packettype_name(IB),                    \
	packettype_name(ERROR),                 \
	packettype_name(BYPASS))

#undef TRACE_SYSTEM
#define TRACE_SYSTEM hfi1_rx

TRACE_EVENT(hfi2_rcvhdr,
	    TP_PROTO(struct hfi_devdata *dd,
		     u64 ctxt,
		     u64 eflags,
		     u32 etype,
		     u32 port,
		     u32 hlen,
		     u32 tlen,
		     u32 egr_idx,
		     u32 egr_off
	    ),
	    TP_ARGS(dd, ctxt, eflags, etype, port, hlen, tlen, egr_idx,
		    egr_off),
	    TP_STRUCT__entry(
		DD_DEV_ENTRY(dd)
		__field(u64, ctxt)
		__field(u64, eflags)
		__field(u32, etype)
		__field(u32, port)
		__field(u32, hlen)
		__field(u32, tlen)
		__field(u32, egr_idx)
		__field(u32, egr_off)
	    ),
	    TP_fast_assign(
		DD_DEV_ASSIGN(dd);
		__entry->ctxt = ctxt;
		__entry->eflags = eflags;
		__entry->etype = etype;
		__entry->port = port;
		__entry->hlen = hlen;
		__entry->tlen = tlen;
		__entry->egr_idx = egr_idx;
		__entry->egr_off = egr_off;
		),
	    TP_printk(
"[%s] ctxt 0x%llx eflags 0x%llx etype %d,%s hlen %d tlen %d egr idx %d off %d",
		__get_str(dev),
		__entry->ctxt,
		__entry->eflags,
		__entry->etype, show_packettype(__entry->etype),
		__entry->hlen,
		__entry->tlen,
		__entry->egr_idx,
		__entry->egr_off
	    )
);

#undef TRACE_SYSTEM
#define TRACE_SYSTEM hfi2_irq

DECLARE_EVENT_CLASS(hfi2_irq,
		    TP_PROTO(struct hfi_irq_entry *me),
		    TP_ARGS(me),
		    TP_STRUCT__entry(
			    __field(u32, unit)
			    __field(u32, src)
		    ),
		    TP_fast_assign(
			    __entry->unit = me->dd->unit;
			    __entry->src = me->intr_src;
		    ),
		    TP_printk("[%d] src: %d", __entry->unit, __entry->src)
);

DEFINE_EVENT(hfi2_irq, hfi2_irq_eq,
	     TP_PROTO(struct hfi_irq_entry *me),
	     TP_ARGS(me)
);
DEFINE_EVENT(hfi2_irq, hfi2_irq_phy,
	     TP_PROTO(struct hfi_irq_entry *me),
	     TP_ARGS(me)
);
DEFINE_EVENT(hfi2_irq, hfi2_irq_err,
	     TP_PROTO(struct hfi_irq_entry *me),
	     TP_ARGS(me)
);

#undef TRACE_SYSTEM
#define TRACE_SYSTEM hfi1_tx

#define wr_opcode_name(opcode) { IB_WR_##opcode, #opcode  }
#define show_wr_opcode(opcode)                             \
__print_symbolic(opcode,                                   \
	wr_opcode_name(RDMA_WRITE),                        \
	wr_opcode_name(RDMA_WRITE_WITH_IMM),               \
	wr_opcode_name(SEND),                              \
	wr_opcode_name(SEND_WITH_IMM),                     \
	wr_opcode_name(RDMA_READ),                         \
	wr_opcode_name(ATOMIC_CMP_AND_SWP),                \
	wr_opcode_name(ATOMIC_FETCH_AND_ADD),              \
	wr_opcode_name(LSO),                               \
	wr_opcode_name(SEND_WITH_INV),                     \
	wr_opcode_name(RDMA_READ_WITH_INV),                \
	wr_opcode_name(LOCAL_INV),                         \
	wr_opcode_name(REG_MR),                            \
	wr_opcode_name(MASKED_ATOMIC_CMP_AND_SWP),         \
	wr_opcode_name(MASKED_ATOMIC_FETCH_AND_ADD),       \
	wr_opcode_name(REG_SIG_MR))

#define POS_PRN \
"wr_id: %llx qpn: %x, psn: 0x%x, lpsn: 0x%x, length: %u opcode: 0x%.2x,%s, size: %u, head: %u, last: %u"

TRACE_EVENT(hfi2_post_one_send,
	    TP_PROTO(struct rvt_qp *qp, struct rvt_swqe *wqe),
	    TP_ARGS(qp, wqe),
	    TP_STRUCT__entry(
		    __field(u64, wr_id)
		    __field(u32, qpn)
		    __field(u32, psn)
		    __field(u32, lpsn)
		    __field(u32, length)
		    __field(u32, opcode)
		    __field(u32, size)
		    __field(u32, head)
		    __field(u32, last)
		    ),
	    TP_fast_assign(
		    __entry->wr_id = wqe->wr.wr_id;
		    __entry->qpn = qp->ibqp.qp_num;
		    __entry->psn = wqe->psn;
		    __entry->lpsn = wqe->lpsn;
		    __entry->length = wqe->length;
		    __entry->opcode = wqe->wr.opcode;
		    __entry->size = qp->s_size;
		    __entry->head = qp->s_head;
		    __entry->last = qp->s_last;
		    ),
	    TP_printk(POS_PRN,
		      __entry->wr_id,
		      __entry->qpn,
		      __entry->psn,
		      __entry->lpsn,
		      __entry->length,
		      __entry->opcode, show_wr_opcode(__entry->opcode),
		      __entry->size,
		      __entry->head,
		      __entry->last
		)
);

#undef TRACE_SYSTEM
#define TRACE_SYSTEM hfi1_ibhdrs
#if 1

u8 ibhdr_exhdr_len(union hfi2_packet_header *hdr, bool use_16b);
const char *parse_everbs_hdrs(struct trace_seq *p, u8 opcode, void *ehdrs);

#define ibhdr_get_packet_type_str(l4) ((l4) ? "16B" : "9B")
#define __parse_ib_ehdrs(op, ehdrs) parse_everbs_hdrs(p, op, ehdrs)

#define lrh_name(lrh) { HFI1_##lrh, #lrh }
#define show_lnh(lrh)                    \
__print_symbolic(lrh,                    \
	lrh_name(LRH_BTH),               \
	lrh_name(LRH_GRH))

#define ib_opcode_name(opcode) { IB_OPCODE_##opcode, #opcode  }
#define show_ib_opcode(opcode)                             \
__print_symbolic(opcode,                                   \
	ib_opcode_name(RC_SEND_FIRST),                     \
	ib_opcode_name(RC_SEND_MIDDLE),                    \
	ib_opcode_name(RC_SEND_LAST),                      \
	ib_opcode_name(RC_SEND_LAST_WITH_IMMEDIATE),       \
	ib_opcode_name(RC_SEND_ONLY),                      \
	ib_opcode_name(RC_SEND_ONLY_WITH_IMMEDIATE),       \
	ib_opcode_name(RC_RDMA_WRITE_FIRST),               \
	ib_opcode_name(RC_RDMA_WRITE_MIDDLE),              \
	ib_opcode_name(RC_RDMA_WRITE_LAST),                \
	ib_opcode_name(RC_RDMA_WRITE_LAST_WITH_IMMEDIATE), \
	ib_opcode_name(RC_RDMA_WRITE_ONLY),                \
	ib_opcode_name(RC_RDMA_WRITE_ONLY_WITH_IMMEDIATE), \
	ib_opcode_name(RC_RDMA_READ_REQUEST),              \
	ib_opcode_name(RC_RDMA_READ_RESPONSE_FIRST),       \
	ib_opcode_name(RC_RDMA_READ_RESPONSE_MIDDLE),      \
	ib_opcode_name(RC_RDMA_READ_RESPONSE_LAST),        \
	ib_opcode_name(RC_RDMA_READ_RESPONSE_ONLY),        \
	ib_opcode_name(RC_ACKNOWLEDGE),                    \
	ib_opcode_name(RC_ATOMIC_ACKNOWLEDGE),             \
	ib_opcode_name(RC_COMPARE_SWAP),                   \
	ib_opcode_name(RC_FETCH_ADD),                      \
	ib_opcode_name(UC_SEND_FIRST),                     \
	ib_opcode_name(UC_SEND_MIDDLE),                    \
	ib_opcode_name(UC_SEND_LAST),                      \
	ib_opcode_name(UC_SEND_LAST_WITH_IMMEDIATE),       \
	ib_opcode_name(UC_SEND_ONLY),                      \
	ib_opcode_name(UC_SEND_ONLY_WITH_IMMEDIATE),       \
	ib_opcode_name(UC_RDMA_WRITE_FIRST),               \
	ib_opcode_name(UC_RDMA_WRITE_MIDDLE),              \
	ib_opcode_name(UC_RDMA_WRITE_LAST),                \
	ib_opcode_name(UC_RDMA_WRITE_LAST_WITH_IMMEDIATE), \
	ib_opcode_name(UC_RDMA_WRITE_ONLY),                \
	ib_opcode_name(UC_RDMA_WRITE_ONLY_WITH_IMMEDIATE), \
	ib_opcode_name(UD_SEND_ONLY),                      \
	ib_opcode_name(UD_SEND_ONLY_WITH_IMMEDIATE),       \
	ib_opcode_name(CNP))

#define LRH_PRN "len:%d sc:%d dlid:0x%.4x slid:0x%.4x"
#define LRH_9B_PRN "lnh:%d,%s lver:%d sl:%d "
#define LRH_16B_PRN "age:%d l4:%d rc:%d entropy:%d"
#define BTH_PRN \
	"op:0x%.2x,%s se:%d m:%d pad:%d tver:%d pkey:0x%.4x " \
	"becn:%d fecn:%d qpn:0x%.6x a:%d psn:0x%.8x"
#define EHDR_PRN "hlen:%d %s"

DECLARE_EVENT_CLASS(
	hfi2_hdr_template,
	TP_PROTO(struct hfi_devdata *dd,
		 union hfi2_packet_header *hdr,
		 bool use_16b
	),
	TP_ARGS(dd, hdr, use_16b),
	TP_STRUCT__entry(
		DD_DEV_ENTRY(dd)
		/* 9B LRH */
		__field(u8, lnh)
		__field(u8, lver)
		__field(u8, sl)

		/* 16B LRH */
		__field(u8, age)
		__field(u8, l4)
		__field(u8, rc)
		__field(u16, entropy)

		/* 9B and 16B LRH */
		__field(u16, len)
		__field(u32, dlid)
		__field(u8, sc)
		__field(u32, slid)

		/* BTH */
		__field(u8, opcode)
		__field(u8, se)
		__field(u8, m)
		__field(u8, pad)
		__field(u8, tver)
		__field(u16, pkey)
		__field(u8, fecn)
		__field(u8, becn)
		__field(u32, qpn)
		__field(u8, a)
		__field(u32, psn)
		/* extended headers */
		__field(u8, hlen)
		__dynamic_array(u8, ehdrs, ibhdr_exhdr_len(hdr, use_16b))
	),
	TP_fast_assign(
		struct ib_l4_headers *ohdr;

		DD_DEV_ASSIGN(dd);
		/* LRH */
		if (use_16b) {
			/* zero out 9B only fields */
			__entry->lnh = 0;
			__entry->lver = 0;
			__entry->sl = 0;

			opa_parse_16b_header(
				(u32 *)&hdr->opa16b,
				&__entry->slid,
				&__entry->dlid,
				&__entry->len,
				&__entry->pkey,
				&__entry->entropy,
				&__entry->sc,
				&__entry->rc,
				(bool *)&__entry->fecn,
				(bool *)&__entry->becn,
				&__entry->age,
				&__entry->l4);

			if (__entry->l4 == HFI1_L4_IB_LOCAL)
				ohdr = &hdr->opa16b.u.oth;
			else
				ohdr = &hdr->opa16b.u.l.oth;
			__entry->m = (be32_to_cpu(ohdr->bth[1]) >> 31) & 1;
			__entry->pad = (be32_to_cpu(ohdr->bth[0]) >> 20) & 7;
		} else {
			struct hfi2_ib_header *ibh;

			ibh = &hdr->ibh;
			__entry->sc = (u8)(be16_to_cpu(ibh->lrh[0]) >> 12);
			__entry->lver =
				(u8)(be16_to_cpu(ibh->lrh[0]) >> 8) & 0xf;
			__entry->sl = (u8)(be16_to_cpu(ibh->lrh[0]) >> 4) & 0xf;
			__entry->lnh = (u8)(be16_to_cpu(ibh->lrh[0]) & 3);
			__entry->dlid = be16_to_cpu(ibh->lrh[1]);
			__entry->len = be16_to_cpu(ibh->lrh[2]);
			__entry->slid = be16_to_cpu(ibh->lrh[3]);

			/* zero out 16B only fields */
			__entry->age = 0;
			__entry->l4 = 0;
			__entry->rc = 0;
			__entry->entropy = 0;

			if (__entry->lnh == HFI1_LRH_BTH)
				ohdr = &ibh->u.oth;
			else
				ohdr = &ibh->u.l.oth;

			__entry->pkey =	be32_to_cpu(ohdr->bth[0]) & 0xffff;
			__entry->m = (be32_to_cpu(ohdr->bth[0]) >> 22) & 1;
			__entry->pad = (be32_to_cpu(ohdr->bth[0]) >> 20) & 3;
			__entry->fecn =
				(be32_to_cpu(ohdr->bth[1]) >>
					HFI1_FECN_SHIFT)
					& HFI1_FECN_MASK;
			__entry->becn =
				(be32_to_cpu(ohdr->bth[1]) >>
					HFI1_BECN_SHIFT)
					& HFI1_BECN_MASK;
		}
		__entry->opcode = (be32_to_cpu(ohdr->bth[0]) >> 24) & 0xff;
		__entry->se = (be32_to_cpu(ohdr->bth[0]) >> 23) & 1;
		__entry->tver = (be32_to_cpu(ohdr->bth[0]) >> 16) & 0xf;

		__entry->qpn = be32_to_cpu(ohdr->bth[1]) & HFI1_QPN_MASK;
		__entry->a = (be32_to_cpu(ohdr->bth[2]) >> 31) & 1;
		__entry->psn = be32_to_cpu(ohdr->bth[2]) & 0x7fffffff;

		/* extended headers */
		__entry->hlen = ibhdr_exhdr_len(hdr, use_16b);
		memcpy(__get_dynamic_array(ehdrs), &ohdr->u, __entry->hlen);
	),
	TP_printk("[%s] (%s) " LRH_PRN " " LRH_9B_PRN " "
		LRH_16B_PRN " " BTH_PRN " " EHDR_PRN,
		__get_str(dev),
		ibhdr_get_packet_type_str(__entry->l4),
		/* 9B and 16B LRH */
		__entry->len,
		__entry->sc,
		__entry->dlid,
		__entry->slid,
		/* 9B LRH */
		__entry->lnh, show_lnh(__entry->lnh),
		__entry->lver,
		__entry->sl,
		/* 16B LRH */
		__entry->age,
		__entry->l4,
		__entry->rc,
		__entry->entropy,
		/* BTH */
		__entry->opcode, show_ib_opcode(__entry->opcode),
		__entry->se,
		__entry->m,
		__entry->pad,
		__entry->tver,
		__entry->pkey,
		__entry->fecn,
		__entry->becn,
		__entry->qpn,
		__entry->a,
		__entry->psn,
		/* extended headers */
		__entry->hlen,
		__parse_ib_ehdrs(
			__entry->opcode,
			(void *)__get_dynamic_array(ehdrs))
	 )
);

DEFINE_EVENT(hfi2_hdr_template, hfi2_hdr_rcv,
	     TP_PROTO(struct hfi_devdata *dd,
		      union hfi2_packet_header *hdr,
		      bool use_16b),
	     TP_ARGS(dd, hdr, use_16b));
DEFINE_EVENT(hfi2_hdr_template, hfi2_hdr_send_wqe,
	     TP_PROTO(struct hfi_devdata *dd,
		      union hfi2_packet_header *hdr,
		      bool use_16b),
	     TP_ARGS(dd, hdr, use_16b));

DEFINE_EVENT(hfi2_hdr_template, hfi2_hdr_send_ack,
	     TP_PROTO(struct hfi_devdata *dd,
		      union hfi2_packet_header *hdr,
		      bool use_16b),
	     TP_ARGS(dd, hdr, use_16b));
#endif

#undef TRACE_SYSTEM

#define TRACE_SYSTEM hfi2_mad
#define BCT_FORMAT \
	"[%d:%d] shared_limit %x vls 0-7 [%x,%x][%x,%x][%x,%x][%x,%x][%x,%x][%x,%x][%x,%x][%x,%x] 15 [%x,%x]"
#define BCT(field) be16_to_cpu(__entry->bc.field)

DECLARE_EVENT_CLASS(hfi2_bct_template,
		    TP_PROTO(u8 unit,
			     u8 port,
			     struct buffer_control *bc),
		    TP_ARGS(unit, port, bc),
		    TP_STRUCT__entry(__field(u8, unit)
				     __field(u8, port)
				     __field_struct(struct buffer_control, bc)
		    ),
		    TP_fast_assign(__entry->unit = unit;
				   __entry->port = port;
				   __entry->bc = *bc;
		    ),
		    TP_printk(BCT_FORMAT,
			      __entry->unit,
			      __entry->port,
			      BCT(overall_shared_limit),

			      BCT(vl[0].dedicated),
			      BCT(vl[0].shared),

			      BCT(vl[1].dedicated),
			      BCT(vl[1].shared),

			      BCT(vl[2].dedicated),
			      BCT(vl[2].shared),

			      BCT(vl[3].dedicated),
			      BCT(vl[3].shared),

			      BCT(vl[4].dedicated),
			      BCT(vl[4].shared),

			      BCT(vl[5].dedicated),
			      BCT(vl[5].shared),

			      BCT(vl[6].dedicated),
			      BCT(vl[6].shared),

			      BCT(vl[7].dedicated),
			      BCT(vl[7].shared),

			      BCT(vl[15].dedicated),
			      BCT(vl[15].shared)
		    )
);

DEFINE_EVENT(hfi2_bct_template, hfi2_mad_bct_set,
	     TP_PROTO(u8 unit, u8 port, struct buffer_control *bc),
	     TP_ARGS(unit, port, bc));

DEFINE_EVENT(hfi2_bct_template, hfi2_mad_bct_get,
	     TP_PROTO(u8 unit, u8 port, struct buffer_control *bc),
	     TP_ARGS(unit, port, bc));

#define MAD_FORMAT "[%d:%d] mad bver: %2x, cl: %2x, clver: %2x, method: %2x, st: %4x, attr: %4x, tid: %llx"

DECLARE_EVENT_CLASS(hfi2_mad_template,
		    TP_PROTO(u8 unit,
			     u8 port,
			     const struct ib_mad_hdr *mad
		    ),
		    TP_ARGS(unit, port, mad),
		    TP_STRUCT__entry(__field(u8, unit)
				     __field(u8, port)
				     __field_struct(struct ib_mad_hdr, mad)
		    ),
		    TP_fast_assign(__entry->unit = unit;
				   __entry->port = port;
				   __entry->mad = *mad;
		    ),
		    TP_printk(MAD_FORMAT,
			      __entry->unit,
			      __entry->port,
			      __entry->mad.base_version,
			      __entry->mad.mgmt_class,
			      __entry->mad.class_version,
			      __entry->mad.method,
			      be16_to_cpu(__entry->mad.status),
			      be16_to_cpu(__entry->mad.attr_id),
			      be64_to_cpu(__entry->mad.tid)
		    )
);

DEFINE_EVENT(hfi2_mad_template, hfi2_mad_recv,
	     TP_PROTO(u8 unit, u8 port, const struct ib_mad_hdr *mad),
	     TP_ARGS(unit, port, mad));

DEFINE_EVENT(hfi2_mad_template, hfi2_mad_send,
	     TP_PROTO(u8 unit, u8 port, const struct ib_mad_hdr *mad),
	     TP_ARGS(unit, port, mad));

DEFINE_EVENT(hfi2_mad_template, hfi2_mad_trap,
	     TP_PROTO(u8 unit, u8 port, const struct ib_mad_hdr *mad),
	     TP_ARGS(unit, port, mad));

#undef TRACE_SYSTEM
#define TRACE_SYSTEM hfi2_rc

DECLARE_EVENT_CLASS(hfi2_rc_template,
		    TP_PROTO(struct rvt_qp *qp, u32 psn),
		    TP_ARGS(qp, psn),
		    TP_STRUCT__entry(
			DD_DEV_ENTRY(hfi_dd_from_ibdev(qp->ibqp.device))
			__field(u32, qpn)
			__field(u32, s_flags)
			__field(u32, psn)
			__field(u32, s_psn)
			__field(u32, s_next_psn)
			__field(u32, s_sending_psn)
			__field(u32, s_sending_hpsn)
			__field(u32, r_psn)
			),
		    TP_fast_assign(
			DD_DEV_ASSIGN(hfi_dd_from_ibdev(qp->ibqp.device))
			__entry->qpn = qp->ibqp.qp_num;
			__entry->s_flags = qp->s_flags;
			__entry->psn = psn;
			__entry->s_psn = qp->s_psn;
			__entry->s_next_psn = qp->s_next_psn;
			__entry->s_sending_psn = qp->s_sending_psn;
			__entry->s_sending_hpsn = qp->s_sending_hpsn;
			__entry->r_psn = qp->r_psn;
			),
		    TP_printk(
			"[%s] qpn 0x%x s_flags 0x%x psn 0x%x s_psn 0x%x s_next_psn 0x%x s_sending_psn 0x%x sending_hpsn 0x%x r_psn 0x%x",
			__get_str(dev),
			__entry->qpn,
			__entry->s_flags,
			__entry->psn,
			__entry->s_psn,
			__entry->s_next_psn,
			__entry->s_sending_psn,
			__entry->s_sending_hpsn,
			__entry->r_psn
			)
);

DEFINE_EVENT(hfi2_rc_template, hfi2_rc_sendcomplete,
	     TP_PROTO(struct rvt_qp *qp, u32 psn),
	     TP_ARGS(qp, psn)
);

DEFINE_EVENT(hfi2_rc_template, hfi2_rc_ack,
	     TP_PROTO(struct rvt_qp *qp, u32 psn),
	     TP_ARGS(qp, psn)
);

DEFINE_EVENT(hfi2_rc_template, hfi2_rc_timeout,
	     TP_PROTO(struct rvt_qp *qp, u32 psn),
	     TP_ARGS(qp, psn)
);

DEFINE_EVENT(hfi2_rc_template, hfi2_rc_rcv_error,
	     TP_PROTO(struct rvt_qp *qp, u32 psn),
	     TP_ARGS(qp, psn)
);

/*
 * Note:
 * This produces a REALLY ugly trace in the console output when the string is
 * too long.
 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM hfi1_trace

#define MAX_MSG_LEN 512

DECLARE_EVENT_CLASS(hfi1_trace_template,
		    TP_PROTO(const char *function, struct va_format *vaf),
		    TP_ARGS(function, vaf),
		    TP_STRUCT__entry(
			__string(function, function)
			__dynamic_array(char, msg, MAX_MSG_LEN)
		    ),
		    TP_fast_assign(
			__assign_str(function, function);
			WARN_ON_ONCE(vsnprintf(__get_dynamic_array(msg),
					       MAX_MSG_LEN, vaf->fmt,
					       *vaf->va) >= MAX_MSG_LEN);
		    ),
		    TP_printk("(%s) %s",
			      __get_str(function),
			      __get_str(msg))
);

/*
 * It may be nice to macroize the __hfi1_trace but the va_* stuff requires an
 * actual function to work and can not be in a macro.
 */
#define __hfi1_trace_def(lvl) \
void __hfi1_trace_##lvl(const char *funct, char *fmt, ...);		\
									\
DEFINE_EVENT(hfi1_trace_template, hfi1_ ##lvl,				\
	TP_PROTO(const char *function, struct va_format *vaf),		\
	TP_ARGS(function, vaf))

#define __hfi1_trace_fn(lvl) \
void __hfi1_trace_##lvl(const char *func, char *fmt, ...)		\
{									\
	struct va_format vaf = {					\
		.fmt = fmt,						\
	};								\
	va_list args;							\
									\
	va_start(args, fmt);						\
	vaf.va = &args;							\
	trace_hfi1_ ##lvl(func, &vaf);					\
	va_end(args);							\
	return;								\
}

/*
 * To create a new trace level simply define it below and as a __hfi1_trace_fn
 * in trace.c. This will create all the hooks for calling
 * hfi1_cdbg(LVL, fmt, ...); as well as take care of all
 * the debugfs stuff.
 */
__hfi1_trace_def(PKT);
__hfi1_trace_def(PROC);
__hfi1_trace_def(SDMA);
__hfi1_trace_def(LINKVERB);
__hfi1_trace_def(DEBUG);
__hfi1_trace_def(SNOOP);
__hfi1_trace_def(CNTR);
__hfi1_trace_def(PIO);
__hfi1_trace_def(DC8051);
__hfi1_trace_def(FIRMWARE);
__hfi1_trace_def(RCVCTRL);
__hfi1_trace_def(TID);
__hfi1_trace_def(MMU);

#define hfi1_cdbg(which, fmt, ...) \
	__hfi1_trace_##which(__func__, fmt, ##__VA_ARGS__)

#define hfi1_dbg(fmt, ...) \
	hfi1_cdbg(DEBUG, fmt, ##__VA_ARGS__)

/*
 * Define HFI1_EARLY_DBG at compile time or here to enable early trace
 * messages. Do not check in an enablement for this.
 */

#ifdef HFI1_EARLY_DBG
#define hfi1_dbg_early(fmt, ...) \
	trace_printk(fmt, ##__VA_ARGS__)
#else
#define hfi1_dbg_early(fmt, ...)
#endif

#endif /* __HFI1_TRACE_H */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM hfi2

#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE trace
#include <trace/define_trace.h>
