/* SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB */

/*
 * Copyright (c) 2020 Intel Corporation.  All rights reserved.
 *
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM kmap_thread

#if !defined(_TRACE_KMAP_THREAD_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_KMAP_THREAD_H

#include <linux/tracepoint.h>

DECLARE_EVENT_CLASS(kmap_thread_template,
	TP_PROTO(struct task_struct *tsk, struct page *page,
		 void *caller_addr, int cnt),
	TP_ARGS(tsk, page, caller_addr, cnt),

	TP_STRUCT__entry(
		__field(int, pid)
		__field(struct page *, page)
		__field(void *, caller_addr)
		__field(int, cnt)
	),

	TP_fast_assign(
		__entry->pid = tsk->pid;
		__entry->page = page;
		__entry->caller_addr = caller_addr;
		__entry->cnt = cnt;
	),

	TP_printk("PID %d; (%d) %pS %p",
		__entry->pid,
		__entry->cnt,
		__entry->caller_addr,
		__entry->page
	)
);

DEFINE_EVENT(kmap_thread_template, kmap_thread,
	TP_PROTO(struct task_struct *tsk, struct page *page,
		 void *caller_addr, int cnt),
	TP_ARGS(tsk, page, caller_addr, cnt));

DEFINE_EVENT(kmap_thread_template, kunmap_thread,
	TP_PROTO(struct task_struct *tsk, struct page *page,
		 void *caller_addr, int cnt),
	TP_ARGS(tsk, page, caller_addr, cnt));


#endif /* _TRACE_KMAP_THREAD_H */

#include <trace/define_trace.h>
