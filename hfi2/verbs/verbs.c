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

#include <rdma/ib_mad.h>
#include <rdma/ib_user_verbs.h>
#include <linux/idr.h>
#include <linux/module.h>
#include <linux/utsname.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/delay.h>
#include "opa_core_ib.h"
#include "mad.h"
#include "verbs.h"
#include "packet.h"
#include "../link.h"

static void hfi2_uninit_port(struct hfi2_ibport *ibp);

__be64 hfi2_sys_guid;

/* TODO - to be used in various alloc routines */
/* Maximum number of protection domains to support */
unsigned int hfi2_max_pds = 0xFFFF;
/* Maximum number of address handles to support */
unsigned int hfi2_max_ahs = 0xFFFF;
/* Maximum number of completion queue entries to support */
unsigned int hfi2_max_cqes = 0x2FFFF;
/* Maximum number of completion queues to support */
unsigned int hfi2_max_cqs = 0x1FFFF;
/* Maximum number of QP WRs to support */
unsigned int hfi2_max_qp_wrs = 0x3FFF;
/* Maximum number of QPs to support */
unsigned int hfi2_max_qps = 16384;
/* Maximum number of SGEs to support */
unsigned int hfi2_max_sges = 0x60;
/* Maximum number of multicast groups to support */
unsigned int hfi2_max_mcast_grps = 16384;
/* Maximum number of attached QPs to support */
unsigned int hfi2_max_mcast_qp_attached = 16;
/* Maximum number of SRQs to support */
unsigned int hfi2_max_srqs = 1024;
/* Maximum number of SRQ SGEs to support */
unsigned int hfi2_max_srq_sges = 128;
/* Maximum number of SRQ WRs support */
unsigned int hfi2_max_srq_wrs = 0x1FFFF;
/* LKEY table size in bits (2^n, 1 <= n <= 23) */
unsigned int hfi2_lkey_table_size = 16;

/*
 * Note that it is OK to post send work requests in the SQE and ERR
 * states; qib_do_send() will process them and generate error
 * completions as per IB 1.2 C10-96.
 */
const int ib_qp_state_ops[IB_QPS_ERR + 1] = {
	[IB_QPS_RESET] = 0,
	[IB_QPS_INIT] = HFI1_POST_RECV_OK,
	[IB_QPS_RTR] = HFI1_POST_RECV_OK | HFI1_PROCESS_RECV_OK,
	[IB_QPS_RTS] = HFI1_POST_RECV_OK | HFI1_PROCESS_RECV_OK |
	    HFI1_POST_SEND_OK | HFI1_PROCESS_SEND_OK |
	    HFI1_PROCESS_NEXT_SEND_OK,
	[IB_QPS_SQD] = HFI1_POST_RECV_OK | HFI1_PROCESS_RECV_OK |
	    HFI1_POST_SEND_OK | HFI1_PROCESS_SEND_OK,
	[IB_QPS_SQE] = HFI1_POST_RECV_OK | HFI1_PROCESS_RECV_OK |
	    HFI1_POST_SEND_OK | HFI1_FLUSH_SEND,
	[IB_QPS_ERR] = HFI1_POST_RECV_OK | HFI1_FLUSH_RECV |
	    HFI1_POST_SEND_OK | HFI1_FLUSH_SEND,
};

static const char *get_unit_name(int unit)
{
	static char iname[16];

	snprintf(iname, sizeof(iname), DRIVER_NAME "_%u", unit);
	return iname;
}

static const char *get_card_name(struct rvt_dev_info *rdi)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(&rdi->ibdev);

	return get_unit_name(dd->unit);
}

static struct pci_dev *get_pci_dev(struct rvt_dev_info *rdi)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(&rdi->ibdev);

	return dd->pcidev;
}

/*
 * This is the callback from ib_register_device() when completing the
 * sysfs registration for our ports under /sys/class/infiniband/.
 */
static int port_callback(struct ib_device *ibdev, u8 port_num,
			 struct kobject *kobj)
{
	return 0;
}

static void fill_device_attr(struct hfi2_ibdev *ibd)
{
	struct hfi_devdata *dd = ibd->dd;
	struct ib_device_attr *props = &ibd->rdi.dparms.props;

	memset(props, 0, sizeof(*props));

	props->device_cap_flags = IB_DEVICE_BAD_PKEY_CNTR |
		IB_DEVICE_BAD_QKEY_CNTR | IB_DEVICE_SHUTDOWN_PORT |
		IB_DEVICE_SYS_IMAGE_GUID | IB_DEVICE_RC_RNR_NAK_GEN |
		IB_DEVICE_PORT_ACTIVE_EVENT | IB_DEVICE_SRQ_RESIZE;
	props->page_size_cap = PAGE_SIZE;
#if 0
	props->vendor_id = id.vendor;
		dd->oui1 << 16 | dd->oui2 << 8 | dd->oui3;
#else
	props->vendor_id = dd->bus_id.vendor;
#endif
	props->vendor_part_id = dd->bus_id.device;
	props->hw_ver = dd->bus_id.revision;
	props->sys_image_guid = hfi2_sys_guid;
	props->max_mr_size = ~0ULL;
	props->max_qp = hfi2_max_qps;
	props->max_qp_wr = hfi2_max_qp_wrs;
	props->max_sge = hfi2_max_sges;
	props->max_sge_rd = hfi2_max_sges;
	props->max_cq = hfi2_max_cqs;
	props->max_ah = hfi2_max_ahs;
	props->max_cqe = hfi2_max_cqes;
	props->max_mr = 1 << hfi2_lkey_table_size;
	props->max_fmr = 1 << hfi2_lkey_table_size;
	props->max_map_per_fmr = OPA_IB_MAX_MAP_PER_FMR;
	props->max_pd = hfi2_max_pds;
	props->max_qp_rd_atom = OPA_IB_MAX_RDMA_ATOMIC;
	props->max_qp_init_rd_atom = OPA_IB_INIT_RDMA_ATOMIC;
	/* props->max_res_rd_atom */
	props->max_srq = hfi2_max_srqs;
	props->max_srq_wr = hfi2_max_srq_wrs;
	props->max_srq_sge = hfi2_max_srq_sges;
	/* props->local_ca_ack_delay */
	props->atomic_cap = IB_ATOMIC_GLOB;
	props->max_pkeys = HFI_MAX_PKEYS;
	props->max_mcast_grp = hfi2_max_mcast_grps;
	props->max_mcast_qp_attach = hfi2_max_mcast_qp_attached;
	props->max_total_mcast_qp_attach = props->max_mcast_qp_attach *
					   props->max_mcast_grp;
}

static int hfi2_query_port(struct ib_device *ibdev, u8 port,
			     struct ib_port_attr *props)
{
	struct hfi2_ibport *ibp = to_hfi_ibp(ibdev, port);
	struct hfi_pportdata *ppd;

	if (!ibp)
		return -EINVAL;
	ppd = ibp->ppd;
	memset(props, 0, sizeof(*props));

	/* TODO - first cut at port attributes */

	props->lid = ppd->lid;
	props->lmc = ppd->lmc;
	props->sm_lid = ibp->sm_lid;
	props->sm_sl = ibp->sm_sl;
	props->state = ibp->ppd->lstate;
	props->phys_state = hfi_ibphys_portstate(ibp->ppd);
	props->port_cap_flags = ibp->port_cap_flags;
	props->gid_tbl_len = HFI2_GUIDS_PER_PORT;
	props->max_msg_sz = OPA_IB_MAX_MSG_SZ;
	props->pkey_tbl_len = HFI_MAX_PKEYS;
	props->bad_pkey_cntr = ibp->pkey_violations;
	props->qkey_viol_cntr = ibp->qkey_violations;
	props->active_width = (u8)opa_width_to_ib(ppd->link_width_active);
	props->active_speed = (u8)opa_speed_to_ib(ppd->link_speed_active);
	props->max_vl_num = ppd->vls_supported;
	props->init_type_reply = 0;

	/* Once we are a "first class" citizen and have added the OPA MTUs to
	 * the core we can advertise the larger MTU enum to the ULPs, for now
	 * advertise only 4K.
	 *
	 * Those applications which are either OPA aware or pass the MTU enum
	 * from the Path Records to us will get the new 8k MTU.  Those that
	 * attempt to process the MTU enum may fail in various ways.
	 */
	props->max_mtu = valid_ib_mtu(HFI_DEFAULT_MAX_MTU) ?
			 opa_mtu_to_enum(HFI_DEFAULT_MAX_MTU) : IB_MTU_4096;
	props->active_mtu = valid_ib_mtu(ppd->ibmtu) ?
			    opa_mtu_to_enum(ppd->ibmtu) : props->max_mtu;
	props->subnet_timeout = ibp->subnet_timeout;

	return 0;
}

static int hfi2_modify_device(struct ib_device *ibdev, int device_modify_mask,
			      struct ib_device_modify *device_modify)
{
	struct hfi2_ibdev *ibd = to_hfi_ibd(ibdev);
	unsigned i;
	int ret;

	if (device_modify_mask & ~(IB_DEVICE_MODIFY_SYS_IMAGE_GUID |
				   IB_DEVICE_MODIFY_NODE_DESC)) {
		ret = -EOPNOTSUPP;
		goto bail;
	}

	if (device_modify_mask & IB_DEVICE_MODIFY_NODE_DESC) {
		memcpy(ibdev->node_desc, device_modify->node_desc, 64);
		for (i = 0; i < ibd->num_pports; i++) {
			struct hfi2_ibport *ibp = to_hfi_ibp(ibdev, i + 1);

			hfi2_node_desc_chg(ibp);
		}
	}

	if (device_modify_mask & IB_DEVICE_MODIFY_SYS_IMAGE_GUID) {
		hfi2_sys_guid =
			cpu_to_be64(device_modify->sys_image_guid);
		for (i = 0; i < ibd->num_pports; i++) {
			struct hfi2_ibport *ibp = to_hfi_ibp(ibdev, i + 1);

			hfi2_sys_guid_chg(ibp);
		}
	}

	ret = 0;

bail:
	return ret;
}

static int hfi2_modify_port(struct ib_device *ibdev, u8 port,
		       int port_modify_mask, struct ib_port_modify *props)
{
	struct hfi2_ibport *ibp = to_hfi_ibp(ibdev, port);
	struct hfi_pportdata *ppd = ibp->ppd;
	int ret = 0;

	ibp->port_cap_flags |= props->set_port_cap_mask;
	ibp->port_cap_flags &= ~props->clr_port_cap_mask;
	if (props->set_port_cap_mask || props->clr_port_cap_mask)
		hfi2_cap_mask_chg(ibp);
	if (port_modify_mask & IB_PORT_SHUTDOWN) {
		hfi_set_link_down_reason(ppd, OPA_LINKDOWN_REASON_UNKNOWN, 0,
		  OPA_LINKDOWN_REASON_UNKNOWN);
		ret = hfi_set_link_state(ppd, HLS_DN_DOWNDEF);
	}
	if (port_modify_mask & IB_PORT_RESET_QKEY_CNTR)
		ibp->qkey_violations = 0;
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 3, 0)
static int port_immutable(struct ib_device *ibdev, u8 port_num,
			  struct ib_port_immutable *immutable)
{
	struct ib_port_attr attr;
	int err;

	err = hfi2_query_port(ibdev, port_num, &attr);
	if (err)
		return err;

	memset(immutable, 0, sizeof(*immutable));

	immutable->pkey_tbl_len = attr.pkey_tbl_len;
	immutable->gid_tbl_len = attr.gid_tbl_len;
	immutable->core_cap_flags = RDMA_CORE_PORT_INTEL_OPA |
					RDMA_CORE_CAP_OPA_AH;
	immutable->max_mad_size = OPA_MGMT_MAD_SIZE;

	return 0;
}
#endif

static int hfi2_query_pkey(struct ib_device *ibdev, u8 port, u16 index,
			  u16 *pkey)
{
	struct hfi2_ibport *ibp = to_hfi_ibp(ibdev, port);

	if (!ibp || index >= HFI_MAX_PKEYS)
		return -EINVAL;

	*pkey = ibp->ppd->pkeys[index];
	return 0;
}

static int hfi2_query_gid(struct ib_device *ibdev, u8 port,
			  int index, union ib_gid *gid)
{
	struct hfi2_ibport *ibp = to_hfi_ibp(ibdev, port);

	if (!ibp || (index >= HFI2_GUIDS_PER_PORT))
		return -EINVAL;

	gid->global.subnet_prefix = ibp->gid_prefix;
	if (index == 0)
		gid->global.interface_id = ibp->ppd->pguid;
	else
		gid->global.interface_id = ibp->guids[index - 1];

	return 0;
}

static int hfi2_register_device(struct hfi2_ibdev *ibd, const char *name)
{
	struct ib_device *ibdev = &ibd->rdi.ibdev;
	struct hfi2_ibport *ibp;
	int i, ret;
	size_t lcpysz = IB_DEVICE_NAME_MAX;

	lcpysz = strlcpy(ibdev->name, name, lcpysz);
	strlcpy(ibdev->name + lcpysz, "_%d", IB_DEVICE_NAME_MAX - lcpysz);
	strncpy(ibdev->node_desc, init_utsname()->nodename,
		sizeof(ibdev->node_desc));
	ibdev->node_guid = ibd->node_guid;
	/*
	 * The system image GUID is supposed to be the same for all
	 * HFIs in a single system but since there can be other
	 * device types in the system, we can't be sure this is unique.
	 */
	if (!hfi2_sys_guid)
		hfi2_sys_guid = ibdev->node_guid;
	ibdev->owner = THIS_MODULE;
	ibdev->phys_port_cnt = ibd->num_pports;

	ibdev->query_port = hfi2_query_port;
	ibdev->query_pkey = hfi2_query_pkey;
	ibdev->query_gid = hfi2_query_gid;
	ibdev->create_ah = hfi2_create_ah;
	ibdev->modify_ah = hfi2_modify_ah;
	ibdev->query_ah = hfi2_query_ah;
	ibdev->destroy_ah = hfi2_destroy_ah;
	ibdev->create_qp = hfi2_create_qp;
	ibdev->modify_qp = hfi2_modify_qp;
	ibdev->query_qp = hfi2_query_qp;
	ibdev->destroy_qp = hfi2_destroy_qp;
	ibdev->post_send = hfi2_post_send;
	ibdev->post_recv = hfi2_post_receive;
	ibdev->get_dma_mr = hfi2_get_dma_mr;
	ibdev->reg_user_mr = hfi2_reg_user_mr;
	ibdev->dereg_mr = hfi2_dereg_mr;
	ibdev->process_mad = hfi2_process_mad;
	ibdev->dma_device = ibd->parent_dev;
	ibdev->modify_port = hfi2_modify_port;
	ibdev->modify_device = hfi2_modify_device;
	ibdev->attach_mcast = hfi2_multicast_attach;
	ibdev->detach_mcast = hfi2_multicast_detach;
	ibdev->dma_ops = &hfi2_dma_mapping_ops;
	ibdev->get_port_immutable = port_immutable;

	/*
	 * Fill in rvt info object.
	 */
	ibd->rdi.driver_f.port_callback = port_callback;
	ibd->rdi.driver_f.get_card_name = get_card_name;
	ibd->rdi.driver_f.get_pci_dev = get_pci_dev;
#if 0
	ibd->rdi.driver_f.check_ah = hfi1_check_ah;
	ibd->rdi.driver_f.notify_new_ah = hfi1_notify_new_ah;
	ibd->rdi.driver_f.get_guid_be = hfi1_get_guid_be;
	ibd->rdi.driver_f.query_port_state = query_port;
	ibd->rdi.driver_f.shut_down_port = shut_down_port;
	ibd->rdi.driver_f.cap_mask_chg = hfi1_cap_mask_chg;
#endif
	/*
	 * Fill in rvt info device attributes.
	 */
	fill_device_attr(ibd);

#if 0
	/* queue pair */
	dd->verbs_dev.rdi.dparms.qp_table_size = hfi1_qp_table_size;
	dd->verbs_dev.rdi.dparms.qpn_start = 0;
	dd->verbs_dev.rdi.dparms.qpn_inc = 1;
	dd->verbs_dev.rdi.dparms.qos_shift = dd->qos_shift;
	dd->verbs_dev.rdi.dparms.qpn_res_start = kdeth_qp << 16;
	dd->verbs_dev.rdi.dparms.qpn_res_end =
	dd->verbs_dev.rdi.dparms.qpn_res_start + 65535;
	dd->verbs_dev.rdi.dparms.max_rdma_atomic = HFI1_MAX_RDMA_ATOMIC;
	dd->verbs_dev.rdi.dparms.psn_mask = PSN_MASK;
	dd->verbs_dev.rdi.dparms.psn_shift = PSN_SHIFT;
	dd->verbs_dev.rdi.dparms.psn_modify_mask = PSN_MODIFY_MASK;
	dd->verbs_dev.rdi.dparms.core_cap_flags = RDMA_CORE_PORT_INTEL_OPA;
	dd->verbs_dev.rdi.dparms.max_mad_size = OPA_MGMT_MAD_SIZE;

	dd->verbs_dev.rdi.driver_f.qp_priv_alloc = qp_priv_alloc;
	dd->verbs_dev.rdi.driver_f.qp_priv_free = qp_priv_free;
	dd->verbs_dev.rdi.driver_f.free_all_qps = free_all_qps;
	dd->verbs_dev.rdi.driver_f.notify_qp_reset = notify_qp_reset;
	dd->verbs_dev.rdi.driver_f.do_send = hfi1_do_send;
	dd->verbs_dev.rdi.driver_f.schedule_send = hfi1_schedule_send;
	dd->verbs_dev.rdi.driver_f.schedule_send_no_lock = _hfi1_schedule_send;
	dd->verbs_dev.rdi.driver_f.get_pmtu_from_attr = get_pmtu_from_attr;
	dd->verbs_dev.rdi.driver_f.notify_error_qp = notify_error_qp;
	dd->verbs_dev.rdi.driver_f.flush_qp_waiters = flush_qp_waiters;
	dd->verbs_dev.rdi.driver_f.stop_send_queue = stop_send_queue;
	dd->verbs_dev.rdi.driver_f.quiesce_qp = quiesce_qp;
	dd->verbs_dev.rdi.driver_f.notify_error_qp = notify_error_qp;
	dd->verbs_dev.rdi.driver_f.mtu_from_qp = mtu_from_qp;
	dd->verbs_dev.rdi.driver_f.mtu_to_path_mtu = mtu_to_path_mtu;
	dd->verbs_dev.rdi.driver_f.check_modify_qp = hfi1_check_modify_qp;
	dd->verbs_dev.rdi.driver_f.modify_qp = hfi1_modify_qp;
	dd->verbs_dev.rdi.driver_f.check_send_wqe = hfi1_check_send_wqe;
#endif

	/* completion queue */
	snprintf(ibd->rdi.dparms.cq_name, sizeof(ibd->rdi.dparms.cq_name),
		 "hfi2_cq%d", ibd->dd->unit);
	ibd->rdi.dparms.node = ibd->assigned_node_id;

	ibd->rdi.dparms.lkey_table_size = 0; /* disable RDMAVT-MR */
	ibd->rdi.dparms.qp_table_size = 0;   /* disable RDMAVT-QP */
	ibd->rdi.dparms.nports = ibd->num_pports;
	ibd->rdi.dparms.npkeys = HFI_MAX_PKEYS;

	ibp = ibd->pport;
	for (i = 0; i < ibd->num_pports; i++, ibp++)
		rvt_init_port(&ibd->rdi, &ibp->rvp, i, ibp->ppd->pkeys);

	ret = rvt_register_device(&ibd->rdi);
	if (ret)
		goto err_reg;

	/* add optional sysfs files under /sys/class/infiniband/hfi2_0/ */
	hfi2_verbs_register_sysfs(&ibdev->dev);

	return ret;

err_reg:
	dd_dev_err(ibd->dd, "Failed to register with RDMAVT: %d\n", ret);
	return ret;
}

static void hfi2_unregister_device(struct hfi2_ibdev *ibd)
{
	rvt_unregister_device(&ibd->rdi);
}

static int hfi2_init_port(struct hfi2_ibdev *ibd,
			struct hfi_pportdata *ppd)
{
	int ret;
	int pidx = ppd->pnum - 1;
	struct hfi2_ibport *ibp = &ibd->pport[pidx];

	ibp->ibd = ibd;
	ibp->dev = &ibd->rdi.ibdev.dev; /* for dev_info, etc. */
	ibp->ppd = ppd;
	ibp->gid_prefix = IB_DEFAULT_GID_PREFIX;
	ibp->sm_lid = 0;

	ibp->rc_acks = alloc_percpu(u64);
	ibp->rc_qacks = alloc_percpu(u64);
	ibp->rc_delayed_comp = alloc_percpu(u64);
	if (!ibp->rc_acks || !ibp->rc_qacks ||
	    !ibp->rc_delayed_comp) {
		/* error path does any needed free_percpu() */
		ret = -ENOMEM;
		goto percpu_err;
	}

	/* Below should only set bits defined in OPA PortInfo.CapabilityMask */
	ibp->port_cap_flags = IB_PORT_AUTO_MIGR_SUP |
		IB_PORT_CAP_MASK_NOTICE_SUP;
	ibp->port_num = ppd->pnum - 1;

	RCU_INIT_POINTER(ibp->qp[0], NULL);
	RCU_INIT_POINTER(ibp->qp[1], NULL);

	spin_lock_init(&ibp->lock);
	ret = hfi2_ctx_init_port(ibp);
	if (ret < 0)
		goto ctx_init_err;

	/* start RX processing, call this last after no errors */
	hfi2_ctx_start_port(ibp);
	return 0;

ctx_init_err:
percpu_err:
	hfi2_uninit_port(ibp);
	return ret;
}

static void hfi2_uninit_port(struct hfi2_ibport *ibp)
{
	hfi2_ctx_uninit_port(ibp);
	free_percpu(ibp->rc_acks);
	free_percpu(ibp->rc_qacks);
	free_percpu(ibp->rc_delayed_comp);
}

int hfi2_ib_add(struct hfi_devdata *dd, struct opa_core_ops *bus_ops)
{
	int i, ret;
	u8 num_ports;
	struct hfi2_ibdev *ibd;
	struct hfi2_ibport *ibp;

	num_ports = dd->num_pports;
	ibd = (struct hfi2_ibdev *)rvt_alloc_device(sizeof(*ibd) +
						    sizeof(*ibp) * num_ports,
						    num_ports);
	if (!ibd) {
		ret = -ENOMEM;
		goto exit;
	}

	dd->ibd = ibd;
	ibd->dd = dd;
	ibd->num_pports = num_ports;
	ibd->node_guid = dd->nguid;
	memcpy(ibd->oui, dd->oui, ARRAY_SIZE(ibd->oui));
	ibd->assigned_node_id = dd->node;
	ibd->pport = (struct hfi2_ibport *)(ibd + 1);
	ibd->parent_dev = &dd->pcidev->dev;

	/*
	 * Note, ida_simple_get usage restricts itself to QPNs not
	 * reserved for KDETH (PSM).
	 */
	ida_init(&ibd->qpn_even_table);
	ida_init(&ibd->qpn_odd_table);
	idr_init(&ibd->qp_ptr);
	spin_lock_init(&ibd->qpt_lock);
	spin_lock_init(&ibd->n_ahs_lock);
	spin_lock_init(&ibd->n_qps_lock);
	spin_lock_init(&ibd->n_mcast_grps_lock);

	/*
	 * The top hfi2_lkey_table_size bits are used to index the
	 * table.  The lower 8 bits can be owned by the user (copied from
	 * the LKEY).  The remaining bits act as a generation number or tag.
	 */
	spin_lock_init(&ibd->lk_table.lock);
	ibd->lk_table.max = 1 << hfi2_lkey_table_size;
	idr_init(&ibd->lk_table.table);

	/* Allocate Management Context */
	ret = hfi2_ctx_init(ibd, bus_ops);
	if (ret)
		goto ctx_err;

	/* FXRTODO: Move pkey support to hfi2  */
	for (i = 0; i < dd->num_pports; i++) {
		struct hfi_pportdata *ppd = to_hfi_ppd(dd, i + 1);

		ret = hfi2_init_port(ibd, ppd);
		if (ret) {
			while (--i >= 0)
				hfi2_uninit_port(&ibd->pport[i]);
			break;
		}
	}
	if (ret)
		goto port_err;

	/* set RHF event table for Verbs receive */
	ibd->rhf_rcv_functions[RHF_RCV_TYPE_IB] = hfi2_ib_rcv;
	ibd->rhf_rcv_functions[RHF_RCV_TYPE_BYPASS] = hfi2_ib_rcv;
	ibd->rhf_rcv_function_map = ibd->rhf_rcv_functions;
	/* Verbs send functions, can be replaced by snoop */
	ibd->send_wqe = hfi2_send_wqe;
	ibd->send_ack = hfi2_send_ack;

	ret = hfi2_register_device(ibd, hfi_class_name());
	if (ret)
		goto ib_reg_err;
	return ret;
ib_reg_err:
	for (i = 0; i < num_ports; i++)
		hfi2_uninit_port(&ibd->pport[i]);
port_err:
	hfi2_ctx_uninit(ibd);
ctx_err:
	kfree(ibd->rdi.ports); /* TODO - workaround RDMAVT leak */
	ib_dealloc_device(&ibd->rdi.ibdev);
exit:
	/* must clear dd->ibd so state is sane during error cleanup */
	dd->ibd = NULL;
	dev_err(&dd->pcidev->dev, "%s error rc %d\n", __func__, ret);
	return ret;
}

void hfi2_ib_remove(struct hfi_devdata *dd)
{
	int i;
	struct hfi2_ibdev *ibd = dd->ibd;

	if (!dd->ibd)
		return;

	dd->ibd = NULL;
	hfi2_unregister_device(ibd);
	for (i = 0; i < ibd->num_pports; i++)
		hfi2_uninit_port(&ibd->pport[i]);
	hfi2_ctx_uninit(ibd);
	idr_destroy(&ibd->lk_table.table);
	/* TODO - verify empty IDR? */
	idr_destroy(&ibd->qp_ptr);
	ida_destroy(&ibd->qpn_even_table);
	ida_destroy(&ibd->qpn_odd_table);
	/* TODO - workaround for RDMAVT leak */
	kfree(ibd->rdi.ports);
	ib_dealloc_device(&ibd->rdi.ibdev);
}
