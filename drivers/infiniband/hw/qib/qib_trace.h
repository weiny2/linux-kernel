/*
 * Copyright (c) 2014 Intel Corporation. All rights reserved.
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
#if !defined(__QIB_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define __QIB_TRACE_H

#include <linux/tracepoint.h>
#include <linux/trace_seq.h>

#include "qib.h"

/*
 * Note:
 * This produces a REALLY ugly trace in the console output when the string is
 * too long.
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM qib_trace

#define MAX_MSG_LEN 512

DECLARE_EVENT_CLASS(qib_trace_template,
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
 * It may be nice to macroize the __qib_trace but the va_* stuff requires an
 * actual function to work and can not be in a macro. Also the fmt can not be a
 * constant char * because we need to be able to manipulate the \n if it is
 * present.
 */
#define __qib_trace_event(lvl) \
	DEFINE_EVENT(qib_trace_template, qib_ ##lvl,			\
		TP_PROTO(const char *function, struct va_format *vaf),	\
		TP_ARGS(function, vaf))

#ifdef QIB_TRACE_DO_NOT_CREATE_INLINES
#define __qib_trace_fn(fn) __qib_trace_event(fn);
#else
#define __qib_trace_fn(fn) \
__qib_trace_event(fn); \
__printf(2, 3) \
static inline void __qib_trace_ ##fn(const char *func, char *fmt, ...)	\
{									\
	struct va_format vaf = {					\
		.fmt = fmt,						\
	};								\
	va_list args;							\
									\
	va_start(args, fmt);						\
	vaf.va = &args;							\
	trace_qib_ ##fn(func, &vaf);					\
	va_end(args);							\
	return;								\
}
#endif

/*
 * To create a new trace level simply define it as below. This will create all
 * the hooks for calling qib_cdb(LVL, fmt, ...); as well as take care of all
 * the debugfs stuff.
 */
__qib_trace_fn(RVPKT)
__qib_trace_fn(INIT)
__qib_trace_fn(VERB)
__qib_trace_fn(PKT)
__qib_trace_fn(PROC)
__qib_trace_fn(MM)
__qib_trace_fn(ERRPKT)
__qib_trace_fn(SDMA)
__qib_trace_fn(VPKT)
__qib_trace_fn(LINKVERB)
__qib_trace_fn(VERBOSE)
__qib_trace_fn(DEBUG)

#define qib_cdbg(which, fmt, ...) \
	__qib_trace_ ##which(__func__, fmt, ##__VA_ARGS__)

#define qib_dbg(fmt, ...) \
	qib_cdbg(DEBUG, fmt, ##__VA_ARGS__)

#endif /* __QIB_TRACE_H */

#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE qib_trace
#include <trace/define_trace.h>
