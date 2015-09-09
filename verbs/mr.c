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

#include "verbs.h"

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

	if (to_opa_ibpd(pd)->is_user)
		return ERR_PTR(-EPERM);

	mr = kzalloc(sizeof(*mr), GFP_KERNEL);
	if (!mr)
		return ERR_PTR(-ENOMEM);

	ret = &mr->ibmr;
	return ret;
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
	struct opa_ib_mr *mr = NULL;
	struct ib_mr *ret;

	mr = kzalloc(sizeof(*mr), GFP_KERNEL);
	if (!mr)
		return ERR_PTR(-ENOMEM);
	/* TODO lkey */

	ret = &mr->ibmr;
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
	struct opa_ib_mr *mr = NULL;
	struct ib_mr *ret;

	if (length == 0)
		return ERR_PTR(-EINVAL);

	mr = kzalloc(sizeof(*mr), GFP_KERNEL);
	if (!mr)
		return ERR_PTR(-ENOMEM);
	/* TODO lkey */

	ret = &mr->ibmr;
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

	kfree(mr);
	return 0;
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
		   struct ib_sge *isge, struct ib_sge *sge, int acc)
{
	/*
	 * We use LKEY == zero for kernel virtual addresses
	 * (see opa_ib_get_dma_mr and dma.c).
	 */
	rcu_read_lock();
	if (sge->lkey == 0) {
		if (pd->is_user)
			goto bail;
		/* FXRTODO - MR reference count */
		rcu_read_unlock();

		isge->addr = sge->addr;
		isge->length = sge->length;
		goto ok;
	}
	/* FXRTODO - WFR lkey != 0 */
	/* FXRTODO - WFR multi-segment stuff */
	goto bail;

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
int opa_ib_rkey_ok(struct opa_ib_qp *qp, struct ib_sge *sge,
		   u32 len, u64 vaddr, u32 rkey, int acc)
{
	/*
	 * We use RKEY == zero for kernel virtual addresses
	 * (see opa_ib_get_dma_mr and dma.c).

	 */
	rcu_read_lock();
	if (rkey == 0) {
		struct opa_ib_pd *pd = to_opa_ibpd(qp->ibqp.pd);

		if (pd->is_user)
			goto bail;
		/* FXRTODO - MR reference count */
		rcu_read_unlock();

		sge->addr = vaddr;
		sge->length = len;
		goto ok;
	}
	/* FXRTODO - WFR rkey != 0 */
	/* FXRTODO - WFR multi-segment stuff */
	goto bail;

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
