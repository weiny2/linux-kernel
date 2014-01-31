#if !defined(__HFI_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define __HFI_TRACE_H

#include <linux/tracepoint.h>
#include <linux/trace_seq.h>

#include "hfi.h"

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
#define TRACE_SYSTEM hfi_rx

TRACE_EVENT(hfi_rcvhdr,
	TP_PROTO(struct hfi_devdata *dd,
		 u32 ctxt,
		 u32 eflags,
		 u32 etype,
		 u32 tlen,
		 u32 updegr,
		 u32 etail),
	TP_ARGS(dd, ctxt, eflags, etype, tlen, updegr, etail),
	TP_STRUCT__entry(
		DD_DEV_ENTRY(dd)
		__field(u32, ctxt)
		__field(u32, eflags)
		__field(u32, etype)
		__field(u32, tlen)
		__field(u32, updegr)
		__field(u32, etail)
	),
	TP_fast_assign(
		DD_DEV_ASSIGN(dd);
		__entry->ctxt = ctxt;
		__entry->eflags = eflags;
		__entry->etype = etype;
		__entry->tlen = tlen;
		__entry->updegr = updegr;
		__entry->etail = etail;
	),
	TP_printk(
"[%s] ctxt %d eflags 0x%x etype %d,%s tlen %d updegr %d etail %d",
		__get_str(dev),
		__entry->ctxt,
		__entry->eflags,
		__entry->etype, show_packettype(__entry->etype),
		__entry->tlen,
		__entry->updegr,
		__entry->etail
	)
);

TRACE_EVENT(hfi_receive_interrupt,
	TP_PROTO(struct hfi_devdata *dd, u32 ctxt),
	TP_ARGS(dd, ctxt),
	TP_STRUCT__entry(
		DD_DEV_ENTRY(dd)
		__field(u32, ctxt)
	),
	TP_fast_assign(
		DD_DEV_ASSIGN(dd);
		__entry->ctxt = ctxt;
	),
	TP_printk(
		"[%s] ctxt %d",
		__get_str(dev),
		__entry->ctxt
	)
);

#undef TRACE_SYSTEM
#define TRACE_SYSTEM hfi_tx

TRACE_EVENT(hfi_piofree,
	TP_PROTO(struct send_context *sc, int extra),
	TP_ARGS(sc, extra),
	TP_STRUCT__entry(
		DD_DEV_ENTRY(sc->dd)
		__field(u32, ctxt)
		__field(int, extra)
	),
	TP_fast_assign(
		DD_DEV_ASSIGN(sc->dd);
		__entry->ctxt = sc->context;
		__entry->extra = extra;
	),
	TP_printk(
		"[%s] ctxt %d extra %d",
		__get_str(dev),
		__entry->ctxt,
		__entry->extra
	)
);

TRACE_EVENT(hfi_wantpiointr,
	TP_PROTO(struct send_context *sc, u32 needint, u64 credit_ctrl),
	TP_ARGS(sc, needint, credit_ctrl),
	TP_STRUCT__entry(
		DD_DEV_ENTRY(sc->dd)
		__field(u32, ctxt)
		__field(u32, needint)
		__field(u64, credit_ctrl)
	),
	TP_fast_assign(
		DD_DEV_ASSIGN(sc->dd);
		__entry->ctxt = sc->context;
		__entry->needint = needint;
		__entry->credit_ctrl = credit_ctrl;
	),
	TP_printk(
		"[%s] ctxt %d on %d credit_ctrl 0x%llx",
		__get_str(dev),
		__entry->ctxt,
		__entry->needint,
		(unsigned long long)__entry->credit_ctrl
	)
);

DECLARE_EVENT_CLASS(hfi_qpsleepwakeup_template,
	TP_PROTO(struct qib_qp *qp, u32 flags),
	TP_ARGS(qp, flags),
	TP_STRUCT__entry(
		DD_DEV_ENTRY(dd_from_ibdev(qp->ibqp.device))
		__field(u32, qpn)
		__field(u32, flags)
	),
	TP_fast_assign(
		DD_DEV_ASSIGN(dd_from_ibdev(qp->ibqp.device))
		__entry->flags = flags;
		__entry->qpn = qp->ibqp.qp_num;
	),
	TP_printk(
		"[%s] qpn 0x%x flags 0x%x",
		__get_str(dev),
		__entry->qpn,
		__entry->flags
	)
);

DEFINE_EVENT(hfi_qpsleepwakeup_template, hfi_qpwakeup,
	TP_PROTO(struct qib_qp *qp, u32 flags),
	TP_ARGS(qp, flags));

DEFINE_EVENT(hfi_qpsleepwakeup_template, hfi_qpsleep,
	TP_PROTO(struct qib_qp *qp, u32 flags),
	TP_ARGS(qp, flags));

#undef TRACE_SYSTEM
#define TRACE_SYSTEM hfi_ibhdrs

u8 ibhdr_exhdr_len(struct qib_ib_header *hdr);
const char *parse_everbs_hdrs(
	struct trace_seq *p,
	u8 opcode,
	void *ehdrs);

#define __parse_ib_ehdrs(op, ehdrs) parse_everbs_hdrs(p, op, ehdrs)

#define lrh_name(lrh) { QIB_##lrh, #lrh }
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
	ib_opcode_name(UD_SEND_ONLY_WITH_IMMEDIATE))


#define LRH_PRN "vl %d lver %d sl %d lnh %d,%s dlid %.4x len %d slid %.4x"
#define BTH_PRN \
	"op 0x%.2x,%s se %d m %d tver %d pkey 0x%.4x " \
	"qpn 0x%.6x a %d psn 0x%.8x"
#define EHDR_PRN "%s"

DECLARE_EVENT_CLASS(hfi_ibhdr_template,
	TP_PROTO(struct hfi_devdata *dd,
		 struct qib_ib_header *hdr),
	TP_ARGS(dd, hdr),
	TP_STRUCT__entry(
		DD_DEV_ENTRY(dd)
		/* LRH */
		__field(u8, vl)
		__field(u8, lver)
		__field(u8, sl)
		__field(u8, lnh)
		__field(u16, dlid)
		__field(u16, len)
		__field(u16, slid)
		/* BTH */
		__field(u8, opcode)
		__field(u8, se)
		__field(u8, m)
		__field(u8, tver)
		__field(u16, pkey)
		__field(u32, qpn)
		__field(u8, a)
		__field(u32, psn)
		/* extended headers */
		__dynamic_array(u8, ehdrs, ibhdr_exhdr_len(hdr))
	),
	TP_fast_assign(
		struct qib_other_headers *ohdr;
		DD_DEV_ASSIGN(dd);
		/* LRH */
		__entry->vl =
			(u8)(be16_to_cpu(hdr->lrh[0]) >> 12);
		__entry->lver =
			(u8)(be16_to_cpu(hdr->lrh[0]) >> 8) & 0xf;
		__entry->sl =
			(u8)(be16_to_cpu(hdr->lrh[0] >> 4) & 0xf);
		__entry->lnh =
			(u8)(be16_to_cpu(hdr->lrh[0]) & 3);
		__entry->dlid =
			be16_to_cpu(hdr->lrh[1]);
		/* allow for larger len */
		__entry->len =
			be16_to_cpu(hdr->lrh[2]);
		__entry->slid =
			be16_to_cpu(hdr->lrh[3]);
		/* BTH */
		if (__entry->lnh == QIB_LRH_BTH)
			ohdr = &hdr->u.oth;
		else
			ohdr = &hdr->u.l.oth;
		__entry->opcode =
			(be32_to_cpu(ohdr->bth[0]) >> 24) & 0xff;
		__entry->se =
			(be32_to_cpu(ohdr->bth[0]) >> 23) & 1;
		__entry->m =
			 (be32_to_cpu(ohdr->bth[0]) >> 22) & 1;
		__entry->tver =
			(be32_to_cpu(ohdr->bth[0]) >> 16) & 0xf;
		__entry->pkey =
			be32_to_cpu(ohdr->bth[0]) & 0xffff;
		__entry->qpn =
			be32_to_cpu(ohdr->bth[1]) & QIB_QPN_MASK;
		__entry->a =
			be32_to_cpu(ohdr->bth[2] >> 31) & 1;
		/* allow for larger PSN */
		__entry->psn =
			be32_to_cpu(ohdr->bth[2]) & 0x7fffffff;
		/* extended headers */
		 memcpy(
			__get_dynamic_array(ehdrs),
			&ohdr->u,
			ibhdr_exhdr_len(hdr));
	),
	TP_printk("[%s] " LRH_PRN " " BTH_PRN " " EHDR_PRN,
		__get_str(dev),
		/* LRH */
		__entry->vl,
		__entry->lver,
		__entry->sl,
		__entry->lnh, show_lnh(__entry->lnh),
		__entry->dlid,
		__entry->len,
		__entry->slid,
		/* BTH */
		__entry->opcode, show_ib_opcode(__entry->opcode),
		__entry->se,
		__entry->m,
		__entry->tver,
		__entry->pkey,
		__entry->qpn,
		__entry->a,
		__entry->psn,
		/* extended headers */
		__parse_ib_ehdrs(
			__entry->opcode,
			(void *)__get_dynamic_array(ehdrs))
	)
);

DEFINE_EVENT(hfi_ibhdr_template, input_ibhdr,
	TP_PROTO(struct hfi_devdata *dd, struct qib_ib_header *hdr),
	TP_ARGS(dd, hdr));

DEFINE_EVENT(hfi_ibhdr_template, output_ibhdr,
	TP_PROTO(struct hfi_devdata *dd, struct qib_ib_header *hdr),
	TP_ARGS(dd, hdr));

#undef TRACE_SYSTEM
#define TRACE_SYSTEM hfi_ctxts

#define UCTXT_FMT \
	"cred:%u, credaddr:0x%llx, piobase:0x%llx, rcvhdr_cnt:%u, "	\
	"rcvbase:0x%llx, rcvegrc:%u, rcvegrb:0x%llx"
TRACE_EVENT(hfi_uctxtdata,
	    TP_PROTO(struct hfi_devdata *dd, struct qib_ctxtdata *uctxt),
	    TP_ARGS(dd, uctxt),
	    TP_STRUCT__entry(
		    DD_DEV_ENTRY(dd)
		    __field(unsigned, ctxt)
		    __field(u32, credits)
		    __field(u64, hw_free)
		    __field(u64, piobase)
		    __field(u16, rcvhdrq_cnt)
		    __field(u64, rcvhdrq_phys)
		    __field(u32, eager_cnt)
		    __field(u64, rcvegr_phys)
		    ),
	    TP_fast_assign(
		    DD_DEV_ASSIGN(dd);
		    __entry->ctxt = uctxt->ctxt;
		    __entry->credits = uctxt->sc->credits;
		    __entry->hw_free = (u64)uctxt->sc->hw_free;
		    __entry->piobase = (u64)uctxt->sc->base_addr;
		    __entry->rcvhdrq_cnt = uctxt->rcvhdrq_cnt;
		    __entry->rcvhdrq_phys = uctxt->rcvhdrq_phys;
		    __entry->eager_cnt = uctxt->eager_count;
		    __entry->rcvegr_phys = uctxt->rcvegr_phys;
		    ),
	    TP_printk(
		    "[%s] ctxt %u " UCTXT_FMT,
		    __get_str(dev),
		    __entry->ctxt,
		    __entry->credits,
		    __entry->hw_free,
		    __entry->piobase,
		    __entry->rcvhdrq_cnt,
		    __entry->rcvhdrq_phys,
		    __entry->eager_cnt,
		    __entry->rcvegr_phys
		    )
	);

#define CINFO_FMT \
	"egrtids:%u, egr_size:%u, hdrq_cnt:%u, hdrq_size:%u, sdma_ring_size:%u"
TRACE_EVENT(hfi_ctxt_setup,
	    TP_PROTO(struct hfi_devdata *dd, unsigned ctxt, unsigned subctxt,
		     struct hfi_ctxt_setup *cinfo),
	    TP_ARGS(dd, ctxt, subctxt, cinfo),
	    TP_STRUCT__entry(
		    DD_DEV_ENTRY(dd)
		    __field(unsigned, ctxt)
		    __field(unsigned, subctxt)
		    __field(u16, egrtids)
		    __field(u16, rcvhdrq_cnt)
		    __field(u16, rcvhdrq_size)
		    __field(u16, sdma_ring_size)
		    __field(u32, rcvegr_size)
		    ),
	    TP_fast_assign(
		    DD_DEV_ASSIGN(dd);
		    __entry->ctxt = ctxt;
		    __entry->subctxt = subctxt;
		    __entry->egrtids = cinfo->egrtids;
		    __entry->rcvhdrq_cnt = cinfo->rcvhdrq_cnt;
		    __entry->rcvhdrq_size = cinfo->rcvhdrq_entsize;
		    __entry->sdma_ring_size = cinfo->sdma_ring_size;
		    __entry->rcvegr_size = cinfo->rcvegr_size;
		    ),
	    TP_printk(
		    "[%s] ctxt %u:%u " CINFO_FMT,
		    __get_str(dev),
		    __entry->ctxt,
		    __entry->subctxt,
		    __entry->egrtids,
		    __entry->rcvegr_size,
		    __entry->rcvhdrq_cnt,
		    __entry->rcvhdrq_size,
		    __entry->sdma_ring_size
		    )
	);
#endif /* __HFI_TRACE_H */

#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE trace
#include <trace/define_trace.h>
