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
 * opa_ib_create_cq - create a completion queue
 * @ibdev: the device this completion queue is attached to
 * @entries: the minimum size of the completion queue
 * @context: (unused)
 * @udata: user data for libibverbs.so
 *
 * Called by ib_create_cq() in the generic verbs code.
 *
 * Return: a pointer to the completion queue on success, otherwise
 * returns an errno.
 */
struct ib_cq *opa_ib_create_cq(struct ib_device *ibdev, int entries,
			       int comp_vector, struct ib_ucontext *context,
			       struct ib_udata *udata)
{
	struct opa_ib_cq *cq;
	struct ib_cq *ret;

	if (entries < 1 || entries > opa_ib_max_cqes)
		return ERR_PTR(-EINVAL);

	/* Allocate the completion queue structure. */
	cq = kzalloc(sizeof(*cq), GFP_KERNEL);
	if (!cq)
		return ERR_PTR(-ENOMEM);

	/* ib_create_cq() will initialize cq->ibcq except for cq->ibcq.cqe */
	ret = &cq->ibcq;
	return ret;
}

/**
 * opa_ib_destroy_cq - destroy a completion queue
 * @ibcq: the completion queue to destroy.
 *
 * Called by ib_destroy_cq() in the generic verbs code.
 *
 * Return: 0 on success, otherwise returns an errno.
 */
int opa_ib_destroy_cq(struct ib_cq *ibcq)
{
	struct opa_ib_cq *cq = to_opa_ibcq(ibcq);

	kfree(cq);
	return 0;
}

/**
 * opa_ib_poll_cq - poll for work completion entries
 * @ibcq: the completion queue to poll
 * @num_entries: the maximum number of entries to return
 * @entry: pointer to array where work completions are placed
 *
 * This may be called from interrupt context.  Also called by ib_poll_cq()
 * in the generic verbs code.
 *
 * Return: Number of completion entries polled on success, otherwise
 * returns an errno.
 */
int opa_ib_poll_cq(struct ib_cq *ibcq, int num_entries, struct ib_wc *entry)
{
	return -ENOSYS;
}

/**
 * opa_ib_req_notify_cq - change the notification type for a completion queue
 * @ibcq: the completion queue
 * @notify_flags: the type of notification to request
 *
 * This may be called from interrupt context.  Also called by
 * ib_req_notify_cq() in the generic verbs code.
 *
 * Return: 0 on success, otherwise returns an errno.
 */
int opa_ib_req_notify_cq(struct ib_cq *ibcq, enum ib_cq_notify_flags notify_flags)
{
#warning "opa_ib_req_notify_cq not implemented. Fix this"
	/*
	 * TODO: return success to enable ib_* kernel modules and libraries
	 * depending on this to continue thinking that cq is enabled.
	 * This way Dr Route SMPs loopback works.
	 * This function is to be enabled as part of VPD dev task
	 */
	return 0;
}

/**
 * opa_ib_resize_cq - change the size of the CQ
 * @ibcq: the completion queue
 *
 * Return: 0 on success, otherwise returns an errno.
 */
int opa_ib_resize_cq(struct ib_cq *ibcq, int cqe, struct ib_udata *udata)
{
	return -ENOSYS;
}
