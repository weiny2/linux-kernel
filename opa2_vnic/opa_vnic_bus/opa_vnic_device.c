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
 * Intel(R) Omni-Path Virtual Network Controller bus driver
 */
#include <linux/idr.h>
#include <linux/module.h>
#include <linux/opa_vnic.h>

#define OPA_VNIC_ID_HFI_SHFT     16
#define OPA_VNIC_ID_PORT_SHFT     8
#define OPA_VNIC_GET_ID(hfi, port, vport)   (((hfi) << OPA_VNIC_ID_HFI_SHFT) | \
				     ((port) << OPA_VNIC_ID_PORT_SHFT) | vport)


/* Unique numbering for vnic control devices */
static struct ida opa_vnic_ctrl_ida;

/* Unique numbering for vnic devices */
static struct idr opa_vnic_idr;

/* opa vnic device type */
struct device_type opa_vnic_dev_type = {
	.name = "opa_vnic_dev",
};

/* opa vnic control device type */
struct device_type opa_vnic_ctrl_dev_type = {
	.name = "opa_vnic_ctrl_dev",
};

/* opa_vnic_drv_type - get the vnic driver type */
static inline u8 opa_vnic_drv_type(struct device_driver *drv)
{
	return container_of(drv, struct opa_vnic_drvwrap, driver)->type;
}

/* opa_vnic_match_device - device, driver match function */
static int opa_vnic_match_device(struct device *dev, struct device_driver *drv)
{
	if ((dev->type == &opa_vnic_dev_type) &&
	    (opa_vnic_drv_type(drv) == OPA_VNIC_DRV))
		return 1;
	else if ((dev->type == &opa_vnic_ctrl_dev_type) &&
		 (opa_vnic_drv_type(drv) == OPA_VNIC_CTRL_DRV))
		return 1;

	return 0;
}

/* opa vnic bus structure */
static struct bus_type opa_vnic_bus = {
	.name = "opa_vnic_bus",
	.match = opa_vnic_match_device,
};

/* opa_vnic_dev_release - device release function */
static void opa_vnic_dev_release(struct device *dev)
{
	struct opa_vnic_device *vdev = container_of(dev,
					    struct opa_vnic_device, dev);
	kfree(vdev);
}

/* opa_vnic_get_dev - get opa vnic device */
struct opa_vnic_device *opa_vnic_get_dev(struct opa_vnic_ctrl_device *cdev,
					 u8 port_num, u8 vport_num)
{
	int id;

	id = OPA_VNIC_GET_ID(cdev->id, port_num, vport_num);
	return idr_find(&opa_vnic_idr, id);
}
EXPORT_SYMBOL_GPL(opa_vnic_get_dev);

/* opa_vnic_device_register - register opa vnic device */
struct opa_vnic_device *opa_vnic_device_register(
					 struct opa_vnic_ctrl_device *cdev,
					 u8 port_num, u8 vport_num, void *priv,
					 struct opa_vnic_hfi_ops *ops)
{
	struct opa_vnic_device *vdev;
	int id, rc;

	vdev = kzalloc(sizeof(struct opa_vnic_device), GFP_KERNEL);
	if (!vdev)
		return ERR_PTR(-ENOMEM);

	id = OPA_VNIC_GET_ID(cdev->id, port_num, vport_num);
	rc = idr_alloc(&opa_vnic_idr, vdev, id, (id + 1), GFP_NOWAIT);
	if (rc < 0)
		goto idr_err;

	vdev->dev.release = opa_vnic_dev_release;
	vdev->dev.bus = &opa_vnic_bus;

	vdev->id = id;
	vdev->cdev = cdev;
	vdev->bus_ops = ops;
	vdev->hfi_priv = priv;
	dev_set_name(&vdev->dev, "opa_vnic_%02d.%02d.%02d",
		     cdev->id, port_num, vport_num);
	vdev->dev.type = &opa_vnic_dev_type;
	rc = device_register(&vdev->dev);
	if (rc)
		goto dev_err;

	dev_info(&vdev->dev, "added vport\n");
	return vdev;

dev_err:
	idr_remove(&opa_vnic_idr, id);
idr_err:
	kfree(vdev);
	return ERR_PTR(rc);
}
EXPORT_SYMBOL_GPL(opa_vnic_device_register);

/* opa_vnic_device_unregister - unregister opa vnic device */
void opa_vnic_device_unregister(struct opa_vnic_device *vdev)
{
	int id = vdev->id;

	dev_info(&vdev->dev, "removing vport\n");
	device_unregister(&vdev->dev);
	idr_remove(&opa_vnic_idr, id);
}
EXPORT_SYMBOL_GPL(opa_vnic_device_unregister);

/* opa_vnic_driver_register - register opa vnic driver */
int opa_vnic_driver_register(struct opa_vnic_driver *drv)
{
	drv->drvwrap.driver.bus = &opa_vnic_bus;
	return driver_register(&drv->drvwrap.driver);
}
EXPORT_SYMBOL_GPL(opa_vnic_driver_register);

/* opa_vnic_driver_unregister - unregister opa vnic driver */
void opa_vnic_driver_unregister(struct opa_vnic_driver *drv)
{
	driver_unregister(&drv->drvwrap.driver);
}
EXPORT_SYMBOL_GPL(opa_vnic_driver_unregister);

/* opa_vnic_ctrl_dev_release - control device release function */
static void opa_vnic_ctrl_dev_release(struct device *dev)
{
	struct opa_vnic_ctrl_device *cdev = container_of(dev,
					    struct opa_vnic_ctrl_device, dev);
	kfree(cdev);
}

/* opa_vnic_ctrl_device_register - register opa vnic control device */
struct opa_vnic_ctrl_device *opa_vnic_ctrl_device_register(
					   struct ib_device *ibdev,
					   u8 num_ports, void *priv,
					   struct opa_vnic_ctrl_ops *ops)
{
	struct opa_vnic_ctrl_device *cdev;
	int rc;

	cdev = kzalloc(sizeof(struct opa_vnic_ctrl_device), GFP_KERNEL);
	if (!cdev)
		return ERR_PTR(-ENOMEM);

	rc = ida_simple_get(&opa_vnic_ctrl_ida, 0, 0, GFP_KERNEL);
	if (rc < 0)
		goto ida_err;

	cdev->id = rc;
	cdev->dev.release = opa_vnic_ctrl_dev_release;
	cdev->dev.bus = &opa_vnic_bus;

	cdev->ibdev = ibdev;
	cdev->num_ports = num_ports;
	cdev->ctrl_ops = ops;
	cdev->hfi_priv = priv;
	dev_set_name(&cdev->dev, "opa_vnic_ctrl%d", cdev->id);
	cdev->dev.type = &opa_vnic_ctrl_dev_type;
	rc = device_register(&cdev->dev);
	if (rc)
		goto dev_err;

	dev_info(&cdev->dev, "added vnic control port\n");
	return cdev;

dev_err:
	ida_simple_remove(&opa_vnic_ctrl_ida, cdev->id);
ida_err:
	kfree(cdev);
	return ERR_PTR(rc);
}
EXPORT_SYMBOL_GPL(opa_vnic_ctrl_device_register);

/* opa_idr_remove_vnic_port - remove vnic port belonging to a cdev */
static int opa_idr_remove_vnic_port(int id, void *p, void *data)
{
	struct opa_vnic_device *vdev = (struct opa_vnic_device *)p;

	if (vdev->cdev == data)
		opa_vnic_device_unregister(vdev);
	return 0;
}

/* opa_vnic_ctrl_device_unregister - unregister opa vnic control device */
void opa_vnic_ctrl_device_unregister(struct opa_vnic_ctrl_device *cdev)
{
	int id = cdev->id;

	dev_info(&cdev->dev, "removing vnic control port\n");
	idr_for_each(&opa_vnic_idr, opa_idr_remove_vnic_port, cdev);
	device_unregister(&cdev->dev);
	ida_simple_remove(&opa_vnic_ctrl_ida, id);
}
EXPORT_SYMBOL_GPL(opa_vnic_ctrl_device_unregister);

/* opa_vnic_ctrl_driver_register - register opa vnic control driver */
int opa_vnic_ctrl_driver_register(struct opa_vnic_ctrl_driver *drv)
{
	drv->drvwrap.driver.bus = &opa_vnic_bus;
	return driver_register(&drv->drvwrap.driver);
}
EXPORT_SYMBOL_GPL(opa_vnic_ctrl_driver_register);

/* opa_vnic_ctrl_driver_unregister - unregister opa vnic control driver */
void opa_vnic_ctrl_driver_unregister(struct opa_vnic_ctrl_driver *drv)
{
	driver_unregister(&drv->drvwrap.driver);
}
EXPORT_SYMBOL_GPL(opa_vnic_ctrl_driver_unregister);

/* opa_vnic_bus_init - initialize opa vnic bus drvier */
int opa_vnic_bus_init(void)
{
	int rc;

	rc = bus_register(&opa_vnic_bus);
	if (rc) {
		pr_err("opa vnic bus init failed %d\n", rc);
		return rc;
	}

	ida_init(&opa_vnic_ctrl_ida);
	idr_init(&opa_vnic_idr);
	return 0;
}
module_init(opa_vnic_bus_init);

/* opa_vnic_bus_deinit - remove opa vnic bus drvier */
void opa_vnic_bus_deinit(void)
{
	idr_destroy(&opa_vnic_idr);
	ida_destroy(&opa_vnic_ctrl_ida);
	bus_unregister(&opa_vnic_bus);
}
module_exit(opa_vnic_bus_deinit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel(R) Omni-Path Virtual Network Controller bus driver");
