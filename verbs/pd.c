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
 * opa_ib_alloc_pd - create a protection domain
 * @ibdev: the IB device
 * @ucontext: associated user context
 * @udata: associated user data
 *
 * Return: a pointer to the protection domain on success, otherwise
 * returns an errno.
 */
struct ib_pd *opa_ib_alloc_pd(struct ib_device *ibdev,
			      struct ib_ucontext *context,
			      struct ib_udata *udata)
{
	struct opa_ib_pd *pd;
	struct ib_pd *ret;

	pd = kzalloc(sizeof(*pd), GFP_KERNEL);
	if (!pd)
		return ERR_PTR(-ENOMEM);
	pd->is_user = udata != NULL;

	/* ib_alloc_pd() will initialize pd->ibpd. */
	ret = &pd->ibpd;
	return ret;
}

/**
 * opa_ib_dealloc_pd - destroy a protection domain
 * @ibpd: the PD to destroy
 *
 * Return: 0 on success, otherwise returns an errno.
 */
int opa_ib_dealloc_pd(struct ib_pd *ibpd)
{
	struct opa_ib_pd *pd = to_opa_ibpd(ibpd);

	kfree(pd);
	return 0;
}

int opa_ib_check_ah(struct ib_device *ibdev, struct ib_ah_attr *ah_attr)
{
	return 0;
}

/**
 * opa_ib_create_ah - create an address handle
 * @pd: the protection domain
 * @ah_attr: the attributes of the AH
 *
 * This may be called from interrupt context.
 *
 * Return: a pointer to the address handle on success, otherwise
 * returns an errno.
 */
struct ib_ah *opa_ib_create_ah(struct ib_pd *pd,
			       struct ib_ah_attr *ah_attr)
{
	struct opa_ib_ah *ah;
	struct ib_ah *ret;

	if (opa_ib_check_ah(pd->device, ah_attr))
		return ERR_PTR(-EINVAL);

	ah = kzalloc(sizeof(*ah), GFP_ATOMIC);
	if (!ah)
		return ERR_PTR(-ENOMEM);

	ah->attr = *ah_attr;

	/* ib_create_ah() will initialize ah->ibah. */
	ret = &ah->ibah;
	return ret;
}

/**
 * opa_ib_destroy_ah - destroy an address handle
 * @ibah: the AH to destroy
 *
 * This may be called from interrupt context.
 *
 * Return: 0 on success, otherwise returns an errno.
 */
int opa_ib_destroy_ah(struct ib_ah *ibah)
{
	struct opa_ib_ah *ah = to_opa_ibah(ibah);

	kfree(ah);
	return 0;
}

/**
 * opa_ib_modify_ah - modify the attributes of an address handle
 * @ibah: the address handle who's attributes we're modifying
 * @ah_attr: the new attributes
 *
 * Return: 0 on success, otherwise returns an errno.
 */
int opa_ib_modify_ah(struct ib_ah *ibah, struct ib_ah_attr *ah_attr)
{
	struct opa_ib_ah *ah = to_opa_ibah(ibah);

	if (opa_ib_check_ah(ibah->device, ah_attr))
		return -EINVAL;

	ah->attr = *ah_attr;
	return 0;
}

/**
 * opa_ib_query_ah - query the attributes of an address handle
 * @ibah: the address handle who's attributes we want
 * @ah_attr: values returned here for AH attributes
 *
 * Return: 0 on success, otherwise returns an errno.
 */
int opa_ib_query_ah(struct ib_ah *ibah, struct ib_ah_attr *ah_attr)
{
	struct opa_ib_ah *ah = to_opa_ibah(ibah);

	*ah_attr = ah->attr;
	return 0;
}
