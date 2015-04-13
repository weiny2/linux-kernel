/*
 * Copyright (c) 2014, 2015 Intel Corporation. All rights reserved.
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
#include <linux/device.h>
#include <linux/wait.h>

#include "common.h"
#include "iowait.h"

#define EXP_TID_TIDLEN_MASK   0x7FFULL
#define EXP_TID_TIDLEN_SHIFT  0
#define EXP_TID_TIDCTRL_MASK  0x3ULL
#define EXP_TID_TIDCTRL_SHIFT 20
#define EXP_TID_TIDIDX_MASK   0x7FFULL
#define EXP_TID_TIDIDX_SHIFT  22
#define EXP_TID_GET(tid, field)	\
	(((tid) >> EXP_TID_TID##field##_SHIFT) & EXP_TID_TID##field##_MASK)

extern uint extended_psn;

struct hfi_user_sdma_pkt_q {
	struct list_head list;
	unsigned ctxt;
	unsigned subctxt;
	u16 n_max_reqs;
	atomic_t n_reqs;
	u16 reqidx;
	struct hfi_devdata *dd;
	struct kmem_cache *txreq_cache;
	struct user_sdma_request *reqs;
	struct iowait busy;
	unsigned state;
};

struct hfi_user_sdma_comp_q {
	u16 nentries;
	struct hfi_sdma_comp_entry *comps;
};

int hfi_user_sdma_alloc_queues(struct qib_ctxtdata *, struct file *);
int hfi_user_sdma_free_queues(struct hfi_filedata *);
int hfi_user_sdma_process_request(struct file *, struct iovec *, unsigned long,
				  unsigned long *);
