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
 * Intel(R) Omni-Path Virtual Network Interface Controller (VNIC)
 * Ethernet Management Agent (EMA) driver
 */

#include <linux/opa_vnic.h>

#include "opa_vnic_internal.h"

static char opa_vnic_ctrl_driver_name[] = "opa_vnic_ctrl";

/* VNIC_TODO: encap parameters should be set only when the port is closed */

/* opa_vnic_ctrl_drv_probe - contorl device initialization routine */
static int opa_vnic_ctrl_drv_probe(struct device *dev)
{
	struct opa_vnic_ctrl_device *cdev = container_of(dev,
					 struct opa_vnic_ctrl_device, dev);
	int i;

	/* VNIC_TODO: dummy devices; remove later */
	for (i = 0; i < 2; i++) {
		cdev->ctrl_ops->add_vport(cdev, i + 1, 0);
		cdev->ctrl_ops->add_vport(cdev, i + 1, 1);
	}

	dev_info(dev, "initialized\n");
	return 0;
}

/* opa_vnic_ctrl_drv_remove - control device removal routine */
static int opa_vnic_ctrl_drv_remove(struct device *dev)
{
	struct opa_vnic_ctrl_device *cdev = container_of(dev,
					 struct opa_vnic_ctrl_device, dev);
	int i;

	/* VNIC_TODO: dummy devices; remove later */
	for (i = 0; i < 2; i++) {
		cdev->ctrl_ops->rem_vport(cdev, i + 1, 0);
		cdev->ctrl_ops->rem_vport(cdev, i + 1, 1);
	}

	dev_info(dev, "removed\n");
	return 0;
}

/* Omni-Path Virtual Network Contrl Driver */
/* TODO: probably have bus implement driver routines */
static struct opa_vnic_ctrl_driver opa_vnic_ctrl_drv = {
	.drvwrap = {
		.type = OPA_VNIC_CTRL_DRV,
		.driver = {
			.name   = opa_vnic_ctrl_driver_name,
			.probe  = opa_vnic_ctrl_drv_probe,
			.remove = opa_vnic_ctrl_drv_remove
		}
	}
};

/* opa_vnic_vema_init - initialize vema */
int __init opa_vnic_vema_init(void)
{
	int rc;

	rc = opa_vnic_ctrl_driver_register(&opa_vnic_ctrl_drv);
	if (rc)
		pr_err("VNIC ctrl driver register failed %d\n", rc);

	return rc;
}

/* opa_vnic_vema_deinit - deinitialize vema */
void opa_vnic_vema_deinit(void)
{
	opa_vnic_ctrl_driver_unregister(&opa_vnic_ctrl_drv);
}
