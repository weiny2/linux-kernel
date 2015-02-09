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
 * opa_ib_create_qp - create a queue pair for a device
 * @ibpd: the protection domain who's device we create the queue pair for
 * @init_attr: the attributes of the queue pair
 * @udata: user data for libibverbs.so
 *
 * Called by the ib_create_qp() core verbs function.
 *
 * Return: the queue pair on success, otherwise returns an errno.
 */
struct ib_qp *opa_ib_create_qp(struct ib_pd *ibpd,
			       struct ib_qp_init_attr *init_attr,
			       struct ib_udata *udata)
{
	struct opa_ib_qp *qp;
	struct ib_qp *ret;

	qp = kzalloc(sizeof(*qp), GFP_KERNEL);
	if (!qp)
		return ERR_PTR(-ENOMEM);

	/* ib_create_qp() will initialize qp->ibqp except for qp->ibqp.qp_num */
	ret = &qp->ibqp;
	return ret;
}

/**
 * opa_ib_destroy_qp - destroy a queue pair
 * @ibqp: the queue pair to destroy
 *
 * Note, this can be called while the QP is actively sending or receiving!
 *
 * Return: 0 on success, otherwise returns an errno.
 */
int opa_ib_destroy_qp(struct ib_qp *ibqp)
{
	struct opa_ib_qp *qp = to_opa_ibqp(ibqp);

	kfree(qp);
	return 0;
}

/**
 * opa_ib_modify_qp - modify the attributes of a queue pair
 * @ibqp: the queue pair who's attributes we're modifying
 * @attr: the new attributes
 * @attr_mask: the mask of attributes to modify
 * @udata: user data for libibverbs.so
 *
 * Return: 0 on success, otherwise returns an errno.
 */
int opa_ib_modify_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		     int attr_mask, struct ib_udata *udata)
{
	return -ENOSYS;
}

/**
 * opa_ib_query_qp - query the attributes of a queue pair
 * @ibqp: the queue pair who's attributes we want
 * @attr: requested QP attribute values returned here
 * @attr_mask: the mask of attributes to query
 * @init_attr: initialization defaults for QP attributes returned here
 *
 * Return: 0 on success and attributes returned in @attr, otherwise
 * returns an errno.
 */
int opa_ib_query_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		    int attr_mask, struct ib_qp_init_attr *init_attr)
{
	return -ENOSYS;
}
