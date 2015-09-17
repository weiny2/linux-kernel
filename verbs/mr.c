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
 *
 * Intel(R) OPA Gen2 IB Driver
 */

#include <rdma/ib_umem.h>
#include "verbs.h"

static int opa_ib_alloc_lkey(struct hfi2_mregion *mr, int dma_region);
static void opa_ib_free_lkey(struct hfi2_mregion *mr);

static int init_mregion(struct hfi2_mregion *mr, struct ib_pd *pd,
			int count)
{
	int m, i = 0;
	int rval = 0;

	m = (count + HFI2_SEGSZ - 1) / HFI2_SEGSZ;
	for (; i < m; i++) {
		mr->map[i] = kzalloc(sizeof(*mr->map[0]), GFP_KERNEL);
		if (!mr->map[i])
			goto bail;
	}
	mr->mapsz = m;
	init_completion(&mr->comp);
	/* count returning the ptr to user */
	atomic_set(&mr->refcount, 1);
	mr->pd = pd;
	mr->max_segs = count;
out:
	return rval;
bail:
	while (i)
		kfree(mr->map[--i]);
	rval = -ENOMEM;
	goto out;
}

static void deinit_mregion(struct hfi2_mregion *mr)
{
	int i = mr->mapsz;

	mr->mapsz = 0;
	while (i)
		kfree(mr->map[--i]);
}


/**
 * opa_ib_get_dma_mr - get a DMA memory region
 * @pd: protection domain for this memory region
 * @acc: access flags
 *
 * Note that all DMA addresses should be created via the
 * struct ib_dma_mapping_ops functions (see dma.c).
 *
 * Return: the memory region on success, otherwise returns an errno.
 */
struct ib_mr *opa_ib_get_dma_mr(struct ib_pd *pd, int acc)
{
	struct opa_ib_mr *mr = NULL;
	struct ib_mr *ret;
	int rval;

	if (to_opa_ibpd(pd)->is_user) {
		ret = ERR_PTR(-EPERM);
		goto bail;
	}

	mr = kzalloc(sizeof(*mr), GFP_KERNEL);
	if (!mr) {
		ret = ERR_PTR(-ENOMEM);
		goto bail;
	}

	rval = init_mregion(&mr->mr, pd, 0);
	if (rval) {
		ret = ERR_PTR(rval);
		goto bail;
	}

	rval = opa_ib_alloc_lkey(&mr->mr, 1);
	if (rval) {
		ret = ERR_PTR(rval);
		goto bail_mregion;
	}

	mr->mr.access_flags = acc;
	ret = &mr->ibmr;
	return ret;

bail_mregion:
	deinit_mregion(&mr->mr);
bail:
	kfree(mr);
	return ret;
}

static struct opa_ib_mr *alloc_mr(int count, struct ib_pd *pd)
{
	struct opa_ib_mr *mr;
	int rval = -ENOMEM;
	int m;

	/* Allocate struct plus pointers to first level page tables. */
	m = (count + HFI2_SEGSZ - 1) / HFI2_SEGSZ;
	mr = kzalloc(sizeof(*mr) + m * sizeof(mr->mr.map[0]), GFP_KERNEL);
	if (!mr)
		goto bail;

	rval = init_mregion(&mr->mr, pd, count);
	if (rval)
		goto bail;
	/*
	 * ib_reg_phys_mr() will initialize mr->ibmr except for
	 * lkey and rkey.
	 */
	rval = opa_ib_alloc_lkey(&mr->mr, 0);
	if (rval)
		goto bail_mregion;
	mr->ibmr.lkey = mr->mr.lkey;
	mr->ibmr.rkey = mr->mr.lkey;
	return mr;

bail_mregion:
	deinit_mregion(&mr->mr);
bail:
	kfree(mr);
	return ERR_PTR(rval);
}

/**
 * opa_ib_reg_phys_mr - register a physical memory region
 * @pd: protection domain for this memory region
 * @buffer_list: pointer to the list of physical buffers to register
 * @num_phys_buf: the number of physical buffers to register
 * @iova_start: the starting address passed over IB which maps to this MR
 *
 * Return: the memory region on success, otherwise returns an errno.
 */
struct ib_mr *opa_ib_reg_phys_mr(struct ib_pd *pd,
				 struct ib_phys_buf *buffer_list,
				 int num_phys_buf, int acc, u64 *iova_start)
{
	struct opa_ib_mr *mr;
	int n, m, i;
	struct ib_mr *ret;

	mr = alloc_mr(num_phys_buf, pd);
	if (IS_ERR(mr)) {
		ret = (struct ib_mr *)mr;
		goto bail;
	}

	mr->mr.user_base = *iova_start;
	mr->mr.iova = *iova_start;
	mr->mr.access_flags = acc;

	m = 0;
	n = 0;
	for (i = 0; i < num_phys_buf; i++) {
		mr->mr.map[m]->segs[n].vaddr = (void *) buffer_list[i].addr;
		mr->mr.map[m]->segs[n].length = buffer_list[i].size;
		mr->mr.length += buffer_list[i].size;
		n++;
		if (n == HFI2_SEGSZ) {
			m++;
			n = 0;
		}
	}

	ret = &mr->ibmr;

bail:
	return ret;
}

/**
 * opa_ib_reg_user_mr - register a userspace memory region
 * @pd: protection domain for this memory region
 * @start: starting userspace address
 * @length: length of region to register
 * @mr_access_flags: access flags for this memory region
 * @udata: unused by the driver
 *
 * Return: the memory region on success, otherwise returns an errno.
 */
struct ib_mr *opa_ib_reg_user_mr(struct ib_pd *pd, u64 start, u64 length,
				 u64 virt_addr, int mr_access_flags,
				 struct ib_udata *udata)
{
	struct opa_ib_mr *mr;
	struct ib_umem *umem;
	struct ib_umem_chunk *chunk;
	int n, m, i;
	struct ib_mr *ret;

	if (length == 0) {
		ret = ERR_PTR(-EINVAL);
		goto bail;
	}

	umem = ib_umem_get(pd->uobject->context, start, length,
			   mr_access_flags, 0);
	if (IS_ERR(umem))
		return (void *)umem;

	n = 0;
	list_for_each_entry(chunk, &umem->chunk_list, list)
		n += chunk->nents;

	mr = alloc_mr(n, pd);
	if (IS_ERR(mr)) {
		ret = (struct ib_mr *)mr;
		ib_umem_release(umem);
		goto bail;
	}

	mr->mr.user_base = start;
	mr->mr.iova = virt_addr;
	mr->mr.length = length;
	mr->mr.offset = umem->offset;
	mr->mr.access_flags = mr_access_flags;
	mr->umem = umem;

	if (is_power_of_2(umem->page_size))
		mr->mr.page_shift = ilog2(umem->page_size);
	m = 0;
	n = 0;
	list_for_each_entry(chunk, &umem->chunk_list, list) {
		for (i = 0; i < chunk->nents; i++) {
			void *vaddr;

			vaddr = page_address(sg_page(&chunk->page_list[i]));
			if (!vaddr) {
				ret = ERR_PTR(-EINVAL);
				goto bail;
			}
			mr->mr.map[m]->segs[n].vaddr = vaddr;
			mr->mr.map[m]->segs[n].length = umem->page_size;
			n++;
			if (n == HFI2_SEGSZ) {
				m++;
				n = 0;
			}
		}
	}
	ret = &mr->ibmr;

bail:
	return ret;
}

/**
 * opa_ib_dereg_mr - unregister and free a memory region
 * @ibmr: the memory region to free
 *
 * Note that this is called to free MRs created by opa_ib_get_dma_mr()
 * or opa_ib_reg_user_mr().
 *
 * Return: 0 on success, otherwise returns an errno.
 */
int opa_ib_dereg_mr(struct ib_mr *ibmr)
{
	struct opa_ib_mr *mr = to_opa_ibmr(ibmr);
	int ret = 0;
	unsigned long timeout;

	opa_ib_free_lkey(&mr->mr);

	hfi2_put_mr(&mr->mr); /* will set completion if last */
	timeout = wait_for_completion_timeout(&mr->mr.comp,
		5 * HZ);
	if (!timeout) {
		struct opa_ib_data *ibd = to_opa_ibdata(mr->mr.pd->device);

		dev_err(&ibd->odev->dev,
			"opa_ib_dereg_mr timeout mr %p pd %p refcount %u\n",
			mr, mr->mr.pd, atomic_read(&mr->mr.refcount));
		hfi2_get_mr(&mr->mr);
		ret = -EBUSY;
		goto out;
	}
	deinit_mregion(&mr->mr);

	if (mr->umem)
		ib_umem_release(mr->umem);
	kfree(mr);
out:
	return ret;
}

/**
 * opa_ib_alloc_lkey - allocate an lkey
 * @mr: memory region that this lkey protects
 * @dma_region: 0->normal key, 1->restricted DMA key
 *
 * Increments mr reference count as required.
 * Sets the lkey field mr for non-dma regions.
 *
 * Return: 0 if successful, otherwise returns -errno.
 */
static int opa_ib_alloc_lkey(struct hfi2_mregion *mr, int dma_region)
{
	unsigned long flags;
	u32 r;
	int ret;
	struct opa_ib_data *ibd = to_opa_ibdata(mr->pd->device);
	struct opa_ib_lkey_table *rkt = &ibd->lk_table;

	hfi2_get_mr(mr);

	/* special case for dma_mr lkey == 0 */
	if (dma_region) {
		struct hfi2_mregion *tmr;

		spin_lock_irqsave(&rkt->lock, flags);
		tmr = rcu_access_pointer(ibd->dma_mr);
		if (!tmr) {
			rcu_assign_pointer(ibd->dma_mr, mr);
			mr->lkey_published = 1;
		} else {
			hfi2_put_mr(mr);
		}
		spin_unlock_irqrestore(&rkt->lock, flags);
		return 0;
	}

	idr_preload(GFP_KERNEL);
	spin_lock_irqsave(&rkt->lock, flags);
	/* Find the next available LKEY */
	ret = idr_alloc_cyclic(&rkt->table, mr, 0, rkt->max, GFP_NOWAIT);
	if (ret < 0) {
		hfi2_put_mr(mr);
		goto idr_err;
	}
	r = ret;
	ret = 0;

	/*
	 * Make sure lkey is never zero which is reserved to indicate an
	 * unrestricted LKEY.
	 */
	rkt->gen++;
	mr->lkey = (r << (32 - opa_ib_lkey_table_size)) |
		((((1 << (24 - opa_ib_lkey_table_size)) - 1) & rkt->gen)
		 << 8);
	if (mr->lkey == 0) {
		mr->lkey |= 1 << 8;
		rkt->gen++;
	}
	mr->lkey_published = 1;

idr_err:
	spin_unlock_irqrestore(&rkt->lock, flags);
	idr_preload_end();
	return ret;
}

/**
 * opa_ib_free_lkey - free an lkey
 * @mr: mr to free from tables
 */
static void opa_ib_free_lkey(struct hfi2_mregion *mr)
{
	unsigned long flags;
	u32 lkey = mr->lkey;
	u32 r;
	struct opa_ib_data *ibd = to_opa_ibdata(mr->pd->device);
	struct opa_ib_lkey_table *rkt = &ibd->lk_table;
	int freed = 0;

	spin_lock_irqsave(&rkt->lock, flags);
	if (!mr->lkey_published)
		goto out;
	if (lkey == 0) {
		RCU_INIT_POINTER(ibd->dma_mr, NULL);
	} else {
		r = lkey >> (32 - opa_ib_lkey_table_size);
		idr_remove(&rkt->table, r);
	}
	mr->lkey_published = 0;
	freed++;
out:
	spin_unlock_irqrestore(&rkt->lock, flags);
	if (freed) {
		synchronize_rcu();
		hfi2_put_mr(mr);
	}
}

/**
 * opa_ib_lkey_ok - check IB SGE for validity and initialize
 * @rkt: table containing lkey to check SGE against
 * @pd: protection domain
 * @isge: outgoing internal SGE
 * @sge: SGE to check
 * @acc: access flags
 *
 * Check the IB SGE for validity and initialize our internal version
 * of it.  Increments the reference count upon success.
 *
 * Return: 1 if valid and successful, otherwise returns 0.
 */
int opa_ib_lkey_ok(struct opa_ib_lkey_table *rkt, struct opa_ib_pd *pd,
		   struct hfi2_sge *isge, struct ib_sge *sge, int acc)
{
	struct hfi2_mregion *mr;
	u32 r;
	unsigned n, m;
	size_t off;

	/*
	 * We use LKEY == zero for kernel virtual addresses
	 * (see opa_ib_get_dma_mr and dma.c).
	 */
	rcu_read_lock();
	if (sge->lkey == 0) {
		struct opa_ib_data *ibd = to_opa_ibdata(pd->ibpd.device);

		if (pd->is_user)
			goto bail;
		mr = rcu_dereference(ibd->dma_mr);
		if (!mr)
			goto bail;
		atomic_inc(&mr->refcount);
		rcu_read_unlock();

		isge->mr = mr;
		isge->vaddr = (void *)sge->addr;
		isge->length = sge->length;
		isge->sge_length = sge->length;
		isge->m = 0;
		isge->n = 0;
		goto ok;
	}

	r = sge->lkey >> (32 - opa_ib_lkey_table_size);
	mr = idr_find(&rkt->table, r);
	if (unlikely(!mr || mr->lkey != sge->lkey || mr->pd != &pd->ibpd))
		goto bail;

	off = sge->addr - mr->user_base;
	if (unlikely(sge->addr < mr->user_base ||
		     off + sge->length > mr->length ||
		     (mr->access_flags & acc) != acc))
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
		m = entries_spanned_by_off / HFI2_SEGSZ;
		n = entries_spanned_by_off % HFI2_SEGSZ;
	} else {
		m = 0;
		n = 0;
		while (off >= mr->map[m]->segs[n].length) {
			off -= mr->map[m]->segs[n].length;
			n++;
			if (n >= HFI2_SEGSZ) {
				m++;
				n = 0;
			}
		}
	}
	isge->mr = mr;
#if 1
	isge->vaddr = mr->map[m]->segs[n].vaddr + off;
	isge->length = mr->map[m]->segs[n].length - off;
#else
	/* TODO - future user PASID optimization */
	isge->vaddr = (void *)sge->addr;
	isge->length = sge->length;
#endif
	isge->sge_length = sge->length;
	isge->m = m;
	isge->n = n;
ok:
	return 1;
bail:
	rcu_read_unlock();
	return 0;
}

/**
 * opa_ib_rkey_ok - check the IB virtual address, length, and RKEY
 * @qp: qp for validation
 * @sge: SGE state
 * @len: length of data
 * @vaddr: virtual address to place data
 * @rkey: rkey to check
 * @acc: access flags
 *
 * Increments the reference count upon success.
 *
 * Return: 1 if successful, otherwise 0.
 */
int opa_ib_rkey_ok(struct opa_ib_qp *qp, struct hfi2_sge *sge,
		   u32 len, u64 vaddr, u32 rkey, int acc)
{
	struct hfi2_mregion *mr;
	u32 r;
	unsigned n, m;
	size_t off;
	struct opa_ib_data *ibd = to_opa_ibdata(qp->ibqp.device);
	struct opa_ib_lkey_table *rkt = &ibd->lk_table;

	/*
	 * We use RKEY == zero for kernel virtual addresses
	 * (see opa_ib_get_dma_mr and dma.c).
	 */
	rcu_read_lock();
	if (rkey == 0) {
		struct opa_ib_pd *pd = to_opa_ibpd(qp->ibqp.pd);

		if (pd->is_user)
			goto bail;
		mr = rcu_dereference(ibd->dma_mr);
		if (!mr)
			goto bail;
		atomic_inc(&mr->refcount);
		rcu_read_unlock();

		sge->mr = mr;
		sge->vaddr = (void *) vaddr;
		sge->length = len;
		sge->sge_length = len;
		sge->m = 0;
		sge->n = 0;
		goto ok;
	}

	r = rkey >> (32 - opa_ib_lkey_table_size);
	mr = idr_find(&rkt->table, r);
	if (unlikely(!mr || mr->lkey != rkey || qp->ibqp.pd != mr->pd))
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
		m = entries_spanned_by_off / HFI2_SEGSZ;
		n = entries_spanned_by_off % HFI2_SEGSZ;
	} else {
		m = 0;
		n = 0;
		while (off >= mr->map[m]->segs[n].length) {
			off -= mr->map[m]->segs[n].length;
			n++;
			if (n >= HFI2_SEGSZ) {
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

/*
 * Initialize the memory region specified by the work reqeust.
 */
int opa_ib_fast_reg_mr(struct opa_ib_qp *qp, struct ib_send_wr *wr)
{
	return 0;
}
