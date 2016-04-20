/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Intel Corporation nor the names of its
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
 */

/*
 * Intel(R) OPA Gen2 IB Driver
 */

#include <rdma/ib_umem.h>
#include "verbs.h"

/**
 * hfi2_rkey_ok - check the IB virtual address, length, and RKEY
 * @qp: qp for validation
 * @sge: SGE state
 * @len: length of data
 * @vaddr: virtual address to place data
 * @rkey: rkey to check
 * @acc: access flags
 *
 * Increments the reference count upon success.
 * TODO - this function temporary until we replace hfi2_qp with rvt_qp
 *
 * Return: 1 if successful, otherwise 0.
 */
int hfi2_rkey_ok(struct hfi2_qp *qp, struct rvt_sge *sge,
		 u32 len, u64 vaddr, u32 rkey, int acc)
{
	struct rvt_mregion *mr;
	unsigned n, m;
	size_t off;
	struct rvt_pd *pd = ibpd_to_rvtpd(qp->ibqp.pd);
	struct rvt_dev_info *dev = ib_to_rvt(pd->ibpd.device);
	struct rvt_lkey_table *rkt = &dev->lkey_table;

	/*
	 * We use RKEY == zero for kernel virtual addresses
	 * (see rvt_get_dma_mr and dma.c).
	 */
	rcu_read_lock();
	if (rkey == 0) {
		if (pd->user)
			goto bail;
		mr = rcu_dereference(dev->dma_mr);
		if (!mr)
			goto bail;
		atomic_inc(&mr->refcount);
		rcu_read_unlock();

		sge->mr = mr;
		sge->vaddr = (void *)vaddr;
		sge->length = len;
		sge->sge_length = len;
		sge->m = 0;
		sge->n = 0;
		goto ok;
	}

	mr = rcu_dereference(
		rkt->table[(rkey >> (32 - hfi2_lkey_table_size))]);
	if (unlikely(!mr || mr->lkey != rkey || &pd->ibpd != mr->pd))
		goto bail;

	off = vaddr - mr->iova;
	if (unlikely(vaddr < mr->iova || off + len > mr->length ||
		     (mr->access_flags & acc) == 0))
		goto bail;
	atomic_inc(&mr->refcount);
	rcu_read_unlock();

	off += mr->offset;
	if (mr->page_shift) {
		/*
		 * page sizes are uniform power of 2 so no loop is necessary
		 * entries_spanned_by_off is the number of times the loop below
		 * would have executed.
		 */
		size_t entries_spanned_by_off;

		entries_spanned_by_off = off >> mr->page_shift;
		off -= (entries_spanned_by_off << mr->page_shift);
		m = entries_spanned_by_off / RVT_SEGSZ;
		n = entries_spanned_by_off % RVT_SEGSZ;
	} else {
		m = 0;
		n = 0;
		while (off >= mr->map[m]->segs[n].length) {
			off -= mr->map[m]->segs[n].length;
			n++;
			if (n >= RVT_SEGSZ) {
				m++;
				n = 0;
			}
		}
	}
	sge->mr = mr;
	sge->vaddr = mr->map[m]->segs[n].vaddr + off;
	sge->length = mr->map[m]->segs[n].length - off;
	sge->sge_length = len;
	sge->m = m;
	sge->n = n;
ok:
	return 1;
bail:
	rcu_read_unlock();
	return 0;
}
