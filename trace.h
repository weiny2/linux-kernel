#if !defined(__HFI_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define __HFI_TRACE_H

#include <linux/tracepoint.h>

#include "hfi.h"

#define DD_DEV_ENTRY       __string(dev, dev_name(&dd->pcidev->dev))
#define DD_DEV_ASSIGN      __assign_str(dev, dev_name(&dd->pcidev->dev))

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
		DD_DEV_ENTRY
		__field(u32, ctxt)
		__field(u32, eflags)
		__field(u32, etype)
		__field(u32, tlen)
		__field(u32, updegr)
		__field(u32, etail)
	),
	TP_fast_assign(
		DD_DEV_ASSIGN;
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
		__entry->etype,
		show_packettype(__entry->etype),
		__entry->tlen,
		__entry->updegr,
		__entry->etail)
);

#endif /* __HFI_TRACE_H */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE trace
#include <trace/define_trace.h>
