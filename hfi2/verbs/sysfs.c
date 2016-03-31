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
 */

/*
 * Intel(R) OPA Gen2 IB Driver
 */

#include <rdma/ib_verbs.h>
#include <linux/kernel.h>
#include "../opa_hfi.h"
#include "verbs.h"

static ssize_t board_id_show(struct device *device, struct device_attribute *attr,
			char *buf)
{
	struct ib_device *ibd =
		container_of(device, struct ib_device, dev);
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibd);
	int ret;

	if (!dd->boardname)
		ret = -EINVAL;
	else
		ret = scnprintf(buf, PAGE_SIZE, "%s\n", dd->boardname);
	return ret;
}
static DEVICE_ATTR_RO(board_id);

static ssize_t boardversion_show(struct device *device,
				 struct device_attribute *attr, char *buf)
{
	struct ib_device *ibd =
		container_of(device, struct ib_device, dev);
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibd);

	return scnprintf(buf, PAGE_SIZE, "ChipABI %u.%u, ChipRev %x.%x\n",
		HFI2_CHIP_VERS_MAJ, HFI2_CHIP_VERS_MIN,
		dd->bus_id.device, dd->bus_id.revision);
}
static DEVICE_ATTR_RO(boardversion);

static ssize_t serial_show(struct device *device,
			   struct device_attribute *attr, char *buf)
{
	struct ib_device *ibd =
		container_of(device, struct ib_device, dev);
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibd);

	/* show the lower 3 byte with leading 1 byte to make same as WFR */
	return scnprintf(buf, PAGE_SIZE, "0x%08llx\n", dd->nguid & 0xFFFFFF);
}
static DEVICE_ATTR_RO(serial);

static struct attribute *hfi2_dev_attrs[] = {
	&dev_attr_board_id.attr,
	&dev_attr_boardversion.attr,
	&dev_attr_serial.attr,
};
ATTRIBUTE_GROUPS(hfi2_dev);

/* add optional sysfs files under /sys/class/infiniband/hfi2_0/ */
void hfi2_verbs_register_sysfs(struct device *dev)
{
	dev->groups = hfi2_dev_groups;
}
