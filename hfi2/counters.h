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
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */
#include "opa_hfi.h"
#include <rdma/fxr/fxr_linkmux_fpc_defs.h>
#include <rdma/fxr/fxr_linkmux_tp_defs.h>
#include <rdma/fxr/fxr_fc_defs.h>

#define CNTR_NORMAL	0x0
#define CNTR_VL		0x1
#define CNTR_INVALID_VL -1

enum {
	XMIT_DATA = 0,
	XMIT_PKTS,
	RCV_DATA,
	RCV_PKTS,
	MCAST_RCVPKTS,
	MCAST_XMITPKTS,
	XMIT_WAIT,
	RCV_FECN,
	RCV_BECN,
	RCV_CONSTRAINT_ERR,
	REMOTE_PHYSICAL_ERR,
	PORT_RCV_ERR,
	FM_CONFIG_ERR,
	UNCORRECTABLE_ERR,
	XMIT_DATA_VL,
	XMIT_PKTS_VL,
	RCV_DATA_VL,
	RCV_PKTS_VL,
	XMIT_WAIT_VL,
	RCV_FECN_VL,
	RCV_BECN_VL,
	PORT_CNTR_LAST
};

struct cntr_entry {
	char *name;
	u64 csr;
	int offset;
	u8 flags;
	u64 (*rw_cntr)(const struct cntr_entry *, void *context, int vl,
			u64 data);
};

int hfi_init_cntrs(struct hfi_devdata *dd);
u32 hfi_read_portcntrs(struct hfi_pportdata *ppd, char **namep, u64 **cntrp);
void hfi_free_cntrs(struct hfi_devdata *dd);
