#ifndef _OPA2_VNIC_HFI_H
#define _OPA2_VNIC_HFI_H
/*
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Intel Corporation nor the names of its
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
 */

/*
 * This file contains all of the Gen2 OPA VNIC declarations.
 */

#include <rdma/opa_core.h>
#include <linux/opa_vnic.h>

/* vnic hfi internal functions */
int opa2_vnic_hfi_setup(struct opa_core_device *odev);
void opa2_vnic_hfi_cleanup(struct opa_core_device *odev);

int opa2_vnic_hfi_add_vports(struct opa_core_device *odev);
void opa2_vnic_hfi_remove_vports(struct opa_core_device *odev);

/* vnic device bus ops */
int opa2_vnic_hfi_init(struct opa_vnic_device *vdev);
void opa2_vnic_hfi_deinit(struct opa_vnic_device *vdev);
int opa2_vnic_hfi_open(struct opa_vnic_device *vdev, opa_vnic_hfi_evt_cb_fn cb);
void opa2_vnic_hfi_close(struct opa_vnic_device *vdev);
int opa2_vnic_hfi_put_skb(struct opa_vnic_device *vdev, struct sk_buff *skb);
struct sk_buff *opa2_vnic_hfi_get_skb(struct opa_vnic_device *vdev);

#endif /* _OPA2_VNIC_HFI_H */
