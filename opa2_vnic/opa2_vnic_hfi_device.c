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
 * Intel(R) Omni-Path vnic hfi driver device handling functions
 */
#include <linux/idr.h>
#include <linux/module.h>
#include <linux/opa_vnic.h>

#include "opa2_vnic_hfi.h"

uint num_veswports = 2;
module_param(num_veswports, uint, S_IRUGO);
MODULE_PARM_DESC(num_veswports, "number of vESWPorts");

static struct opa_vnic_hfi_ops vnic_ops = {
	opa2_vnic_hfi_init,
	opa2_vnic_hfi_deinit,
	opa2_vnic_hfi_open,
	opa2_vnic_hfi_close,
	opa2_vnic_hfi_put_skb,
	opa2_vnic_hfi_get_skb
};

/* opa2_vnic_hfi_add_vports - Add vport devices */
int opa2_vnic_hfi_add_vports(struct opa_core_device *odev)
{
	struct opa_vnic_device *vdev;
	int i, rc = 0;
	u32 vport_id;

	for (i = 0; i < num_veswports; i++) {
		vport_id = (odev->index * num_veswports) + i + 1;
		vdev = opa_vnic_device_register(vport_id, odev, &vnic_ops);
		if (IS_ERR(vdev)) {
			rc = PTR_ERR(vdev);
			pr_err("error adding vport %d: %d\n", vport_id, rc);
			break;
		}
	}

	return rc;
}

/* opa2_vnic_hfi_remove_vports - remove vport devices */
void opa2_vnic_hfi_remove_vports(struct opa_core_device *odev)
{
	struct opa_vnic_device *vdev;
	u32 vport_id;
	int i;

	for (i = 0; i < num_veswports; i++) {
		vport_id = (odev->index * num_veswports) + i + 1;
		vdev = opa_vnic_get_dev(vport_id);
		opa_vnic_device_unregister(vdev);
	}
}
