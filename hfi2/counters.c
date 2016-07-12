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

#include "counters.h"
#include "trace.h"

static u64 port_access_tp_csr(const struct cntr_entry *entry, void *context,
			      int vl, u64 data)
{
	struct hfi_pportdata *ppd = (struct hfi_pportdata *)context;
	u64 csr = entry->csr;

	if (entry->flags & CNTR_VL) {
		return hfi_read_lm_tp_prf_csr(ppd,
		csr + idx_from_vl(vl) + 1);
	} else {
		return hfi_read_lm_tp_prf_csr(ppd, csr);
	}
}

static u64 port_access_fpc_csr(const struct cntr_entry *entry, void *context,
			       int vl, u64 data)
{
	struct hfi_pportdata *ppd = (struct hfi_pportdata *)context;
	u64 csr = entry->csr;

	if (entry->flags & CNTR_VL) {
		return hfi_read_lm_fpc_prf_per_vl_csr(ppd,
		csr, idx_from_vl(vl));
	} else {
		return hfi_read_lm_fpc_csr(ppd, csr);
	}
}

#define CNTR_ELEM(name, csr, offset, flags, accessor)\
{ \
	name, \
	csr, \
	offset, \
	flags, \
	accessor \
}

#define TP_CNTR(name, counter, flags) \
CNTR_ELEM(#name, \
	  counter, \
	  0, \
	  flags, \
	  port_access_tp_csr)

#define FPC_CNTR(name, counter, flags) \
CNTR_ELEM(#name, \
	  counter, \
	  0, \
	  flags, \
	  port_access_fpc_csr)

static struct cntr_entry port_cntrs[PORT_CNTR_LAST] = {
[XMIT_DATA] = TP_CNTR(XmitFlits, TP_PRF_XMIT_DATA, CNTR_NORMAL),
[XMIT_PKTS] = TP_CNTR(XmitPkts, TP_PRF_XMIT_PKTS, CNTR_NORMAL),
[RCV_DATA] = FPC_CNTR(RcvFlits, FXR_FPC_PRF_PORTRCV_DATA_CNT, CNTR_NORMAL),
[RCV_PKTS] = FPC_CNTR(RcvPkts, FXR_FPC_PRF_PORTRCV_PKT_CNT, CNTR_NORMAL),
[MCAST_RCVPKTS] = FPC_CNTR(RcvMCastPkts, FXR_FPC_PRF_PORTRCV_MCAST_PKT_CNT,
			   CNTR_NORMAL),
[MCAST_XMITPKTS] = TP_CNTR(XmitMCastPkts, TP_PRF_MULTICAST_XMIT_PKTS,
			   CNTR_NORMAL),
[XMIT_WAIT] = TP_CNTR(XmitWait, TP_PRF_XMIT_WAIT, CNTR_NORMAL),
[RCV_FECN] = FPC_CNTR(RcvFecn, FXR_FPC_PRF_PORTRCV_FECN, CNTR_NORMAL),
[RCV_BECN] = FPC_CNTR(RcvBecn, FXR_FPC_PRF_PORTRCV_BECN, CNTR_NORMAL),
[RCV_CONSTRAINT_ERR] = FPC_CNTR(RcvConstraintErr,
				FXR_FPC_ERR_PORTRCV_CONSTRAINT_ERROR,
				CNTR_NORMAL),
[REMOTE_PHYSICAL_ERR] = FPC_CNTR(RemotePhyErr,
				FXR_FPC_ERR_PORTRCV_PHY_REMOTE_ERROR,
				CNTR_NORMAL),
[PORT_RCV_ERR] = FPC_CNTR(PortRcvErr, FXR_FPC_ERR_PORTRCV_ERROR,
			  CNTR_NORMAL),
[FM_CONFIG_ERR] = FPC_CNTR(FmConfigErr, FXR_FPC_ERR_FMCONFIG_ERROR,
			   CNTR_NORMAL),
[UNCORRECTABLE_ERR] = FPC_CNTR(UnCorrectableErr,
			       FXR_FPC_ERR_UNCORRECTABLE_ERROR,
			       CNTR_NORMAL),
[XMIT_DATA_VL] = TP_CNTR(XmitFlits, TP_PRF_XMIT_DATA, CNTR_VL),
[XMIT_PKTS_VL] = TP_CNTR(XmitPkts, TP_PRF_XMIT_PKTS, CNTR_VL),
[RCV_DATA_VL] = FPC_CNTR(RcvFlits, FXR_FPC_PRF_PORT_VL_RCV_DATA_CNT, CNTR_VL),
[RCV_PKTS_VL] = FPC_CNTR(RcvPkts, FXR_FPC_PRF_PORT_VL_RCV_PKT_CNT, CNTR_VL),
[XMIT_WAIT_VL] = TP_CNTR(XmitWait, TP_PRF_XMIT_WAIT, CNTR_VL),
[RCV_FECN_VL] = FPC_CNTR(RcvFecn, FXR_FPC_PRF_PORT_VL_RCV_FECN, CNTR_VL),
[RCV_BECN_VL] = FPC_CNTR(RcvBecn, FXR_FPC_PRF_PORT_VL_RCV_BECN, CNTR_VL),
};

#define C_MAX_NAME 13
int hfi_init_cntrs(struct hfi_devdata *dd)
{
	int i, j;
	size_t sz;
	char *p;
	char name[C_MAX_NAME];
	struct hfi_pportdata *ppd;

	dd->nportcntrs = 0;
	sz = 0;

	for (i = 0; i < PORT_CNTR_LAST; i++) {
		port_cntrs[i].offset = dd->nportcntrs;
		if (port_cntrs[i].flags & CNTR_VL) {
			for (j = 0; j < C_VL_COUNT; j++) {
				sz += snprintf(name, C_MAX_NAME, "%s%d",
						port_cntrs[i].name,
						vl_from_idx(j)) + 1;
				dd->nportcntrs++;
			}
		} else {
			sz += strlen(port_cntrs[i].name) + 1;
			dd->nportcntrs++;
		}
	}

	for (i = 0; i < dd->num_pports; i++) {
		ppd = to_hfi_ppd(dd, i + 1);
		ppd->portcntrs = kcalloc(dd->nportcntrs, sizeof(u64),
					 GFP_KERNEL);
		if (!ppd->portcntrs)
			goto bail;
	}

	/* allocate space for counter names */
	dd->portcntrnameslen = sz;
	dd->portcntrnames = kmalloc(sz, GFP_KERNEL);
	if (!dd->portcntrnames)
		goto bail;

	/* fill in the names */
	for (p = dd->portcntrnames, i = 0; i < PORT_CNTR_LAST; i++) {
		if (port_cntrs[i].flags & CNTR_VL) {
			for (j = 0; j < C_VL_COUNT; j++) {
				snprintf(name, C_MAX_NAME, "%s%d",
					 port_cntrs[i].name,
					 vl_from_idx(j));
				memcpy(p, name, strlen(name));
				p += strlen(name);
				*p++ = '\n';
			}
		} else {
			memcpy(p, port_cntrs[i].name,
			       strlen(port_cntrs[i].name));
			p += strlen(port_cntrs[i].name);
			*p++ = '\n';
		}
	}

	return 0;
bail:
	hfi_free_cntrs(dd);
	return -ENOMEM;
}

u32 hfi_read_portcntrs(struct hfi_pportdata *ppd, char **namep, u64 **cntrp)
{
	int ret;
	u64 val = 0;

	if (namep) {
		ret = ppd->dd->portcntrnameslen;
		*namep = ppd->dd->portcntrnames;
	} else {
		const struct cntr_entry *entry;
		int i, j;

		ret = ppd->dd->nportcntrs * sizeof(u64);
		*cntrp = ppd->portcntrs;

		for (i = 0; i < PORT_CNTR_LAST; i++) {
			entry = &port_cntrs[i];
			if (entry->flags & CNTR_VL) {
				for (j = 0; j < C_VL_COUNT; j++) {
					val = entry->rw_cntr(entry, ppd, j, 0);
					ppd->portcntrs[entry->offset + j] = val;
				}
			} else {
				val = entry->rw_cntr(entry, ppd,
						     CNTR_INVALID_VL, 0);
				ppd->portcntrs[entry->offset] = val;
			}
		}
	}
	return ret;
}

void hfi_free_cntrs(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd;
	int i;

	for (i = 0; i < dd->num_pports; i++) {
		ppd = to_hfi_ppd(dd, i + 1);
		kfree(ppd->portcntrs);
		ppd->portcntrs = NULL;
	}
	kfree(dd->portcntrnames);
	dd->portcntrnames = NULL;
}
