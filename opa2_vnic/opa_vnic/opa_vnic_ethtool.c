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
 * Intel(R) Omni-Path VNIC ethtool functions
 */

#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/opa_vnic.h>

#include "opa_vnic_internal.h"

/* vnic_get_drvinfo - get driver info */
static void vnic_get_drvinfo(struct net_device *netdev,
			     struct ethtool_drvinfo *drvinfo)
{
	struct opa_vnic_adapter *adapter = netdev_priv(netdev);
	struct opa_vnic_device *vdev = adapter->vdev;

	strlcpy(drvinfo->driver, opa_vnic_driver_name, sizeof(drvinfo->driver));
	strlcpy(drvinfo->version, opa_vnic_driver_version,
		sizeof(drvinfo->version));
	strlcpy(drvinfo->bus_info, dev_name(&vdev->dev),
		sizeof(drvinfo->bus_info));
}

/* vnic_get_settings - get settings */
static int vnic_get_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
{
	struct opa_vnic_adapter *adapter = netdev_priv(netdev);

	v_dbg("%s\n", __func__);
	cmd->supported = SUPPORTED_Backplane;
	cmd->advertising = ADVERTISED_Backplane;
	ethtool_cmd_speed_set(cmd, SPEED_UNKNOWN);
	cmd->duplex = DUPLEX_FULL;
	cmd->port = PORT_OTHER;
	cmd->phy_address = 0;
	cmd->transceiver = XCVR_DUMMY1;
	cmd->autoneg = AUTONEG_ENABLE;
	cmd->maxtxpkt = 0;
	cmd->maxrxpkt = 0;

	return 0;
}

/* ethtool ops */
static const struct ethtool_ops opa_vnic_ethtool_ops = {
	.get_drvinfo = vnic_get_drvinfo,
	.get_link = ethtool_op_get_link,
	.get_settings = vnic_get_settings,
};

/* opa_vnic_set_ethtool_ops - set ethtool ops */
void opa_vnic_set_ethtool_ops(struct net_device *ndev)
{
	SET_ETHTOOL_OPS(ndev, &opa_vnic_ethtool_ops);
}
