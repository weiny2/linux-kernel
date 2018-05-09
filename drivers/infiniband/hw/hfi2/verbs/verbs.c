/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015-2018 Intel Corporation.
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
 * Copyright (c) 2015-2018 Intel Corporation.
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
#include <rdma/uverbs_std_types.h>
#include <linux/debugfs.h>
#include <linux/idr.h>
#include <linux/module.h>
#include <linux/utsname.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/delay.h>
#include "mad.h"
#include "verbs.h"
#include "packet.h"
#include "uverbs_obj.h"
#include "../link.h"
#include "../counters.h"

DECLARE_UVERBS_OBJECT_TREE(hfi2_uverbs_objects,
			   &hfi2_object_device,
			   &hfi2_object_ctx,
			   &hfi2_object_cmdq,
			   &hfi2_object_job);

static inline const struct uverbs_object_tree_def *hfi2_get_objects(void)
{
	return &hfi2_uverbs_objects;
}

static void hfi2_uninit_port(struct hfi2_ibport *ibp);

__be64 hfi2_sys_guid;

/* Maximum number of protection domains to support */
static unsigned int hfi2_max_pds = 0xFFFF;
/* Maximum number of address handles to support */
static unsigned int hfi2_max_ahs = 0xFFFF;
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
static unsigned int hfi2_max_srqs = 1024;
/* Maximum number of SRQ SGEs to support */
static unsigned int hfi2_max_srq_sges = 128;
/* Maximum number of SRQ WRs support */
static unsigned int hfi2_max_srq_wrs = 0x1FFFF;
/* LKEY table size in bits (2^n, 1 <= n <= 23) */
unsigned int hfi2_lkey_table_size = 16;
/* Size of QP hash table */
static unsigned int hfi2_qp_table_size = 256;

#ifdef HFI_VERBS_TEST
/* TODO: temporary module parameter for packet drop, will not be upstreamed */
static bool rc_drop_enabled;
module_param(rc_drop_enabled, bool, 0444);
MODULE_PARM_DESC(rc_drop_enabled, "Enable random RC packet drops for stress testing");
#endif

static struct pci_dev *get_pci_dev(struct rvt_dev_info *rdi)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(&rdi->ibdev);

	return dd->pdev;
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
		IB_DEVICE_PORT_ACTIVE_EVENT | IB_DEVICE_SRQ_RESIZE |
		IB_DEVICE_MEM_MGT_EXTENSIONS |
		IB_DEVICE_RDMA_NETDEV_OPA_VNIC;
	props->page_size_cap = PAGE_SIZE;
#if 0
	props->vendor_id = id.vendor;
		dd->oui1 << 16 | dd->oui2 << 8 | dd->oui3;
#else
	props->vendor_id = dd->pdev->vendor;
#endif
	props->vendor_part_id = dd->pdev->device;
	props->hw_ver = dd->pdev->revision;
	props->sys_image_guid = hfi2_sys_guid;
	props->max_mr_size = U64_MAX;
	props->max_fast_reg_page_list_len = UINT_MAX;
	props->max_qp = hfi2_max_qps;
	props->max_qp_wr = hfi2_max_qp_wrs;
	props->max_sge = hfi2_max_sges;
	props->max_sge_rd = hfi2_max_sges;
	props->max_cq = hfi2_max_cqs;
	props->max_ah = hfi2_max_ahs;
	props->max_cqe = hfi2_max_cqes;
	props->max_mr = 1 << hfi2_lkey_table_size;
	props->max_fmr = 1 << hfi2_lkey_table_size;
	props->max_map_per_fmr = HFI2_MAX_MAP_PER_FMR;
	props->max_pd = hfi2_max_pds;
	props->max_qp_rd_atom = HFI2_MAX_RDMA_ATOMIC;
	props->max_qp_init_rd_atom = HFI2_INIT_RDMA_ATOMIC;
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

static int hfi2_query_port(struct rvt_dev_info *rdi, u8 port,
			   struct ib_port_attr *props)
{
	struct ib_device *ibdev = &rdi->ibdev;
	struct hfi2_ibport *ibp = to_hfi_ibp(ibdev, port);
	struct hfi_pportdata *ppd;

	if (!ibp)
		return -EINVAL;

	ppd = ibp->ppd;
	props->lid = ppd->lid;
	props->lmc = ppd->lmc;
	props->pkey_tbl_len = HFI_MAX_PKEYS;
	props->state = hfi_driver_lstate(ppd);
	props->phys_state = hfi_driver_pstate(ppd);
	props->gid_tbl_len = HFI2_GUIDS_PER_PORT;
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
	props->active_mtu = valid_ib_mtu(ppd->max_active_mtu) ?
			    opa_mtu_to_enum(ppd->max_active_mtu) :
			    props->max_mtu;
	props->subnet_timeout = ibp->rvp.subnet_timeout;

	/* sm_lid of 0xFFFF needs special handling so that it can
	 * be differentiated from a permissve LID of 0xFFFF.
	 * We set the grh_required flag here so the SA can program
	 * the DGID in the address handle appropriately
	 */
	if (props->sm_lid == be16_to_cpu(IB_LID_PERMISSIVE))
		props->grh_required = true;

	return 0;
}

static int hfi2_modify_device(struct ib_device *ibdev, int device_modify_mask,
			      struct ib_device_modify *device_modify)
{
	struct hfi2_ibdev *ibd = to_hfi_ibd(ibdev);
	unsigned int i;
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

	if (port_modify_mask & IB_PORT_OPA_MASK_CHG) {
		ibp->rvp.port_cap3_flags |= props->set_port_cap_mask;
		ibp->rvp.port_cap3_flags &= ~props->clr_port_cap_mask;
	} else {
		ibp->rvp.port_cap_flags |= props->set_port_cap_mask;
		ibp->rvp.port_cap_flags &= ~props->clr_port_cap_mask;
	}

	if (props->set_port_cap_mask || props->clr_port_cap_mask)
		hfi2_cap_mask_chg(ibp);
	if (port_modify_mask & IB_PORT_SHUTDOWN) {
		hfi_set_link_down_reason(ppd, OPA_LINKDOWN_REASON_UNKNOWN, 0,
					 OPA_LINKDOWN_REASON_UNKNOWN);
		ret = hfi_set_link_state(ppd, HLS_DN_DOWNDEF);
	}
	if (port_modify_mask & IB_PORT_RESET_QKEY_CNTR)
		ibp->rvp.qkey_violations = 0;
	return ret;
}

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

	gid->global.subnet_prefix = ibp->rvp.gid_prefix;
	gid->global.interface_id = get_sguid(ibp->ppd, index);

	return 0;
}

static struct ib_ucontext *hfi2_alloc_ucontext(struct ib_device *ibdev,
					       struct ib_udata *udata)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct hfi_ibcontext *uc;

	uc = kzalloc(sizeof(*uc), GFP_KERNEL);
	if (!uc)
		return ERR_PTR(-ENOMEM);
	uc->ibuc.device = ibdev;
	uc->ops = dd->core_ops;
	uc->priv = dd;

	mutex_init(&uc->vm_lock);
	/* Setup list to zap vmas on release */
	INIT_LIST_HEAD(&uc->vma_head);

	return &uc->ibuc;
}

static int hfi2_dealloc_ucontext(struct ib_ucontext *uc)
{
	kfree(uc);
	return 0;
}

static struct rdma_hw_stats *hfi2_alloc_hw_stats(struct ib_device *ibdev,
						 u8 port_num)
{
	if (!port_num)
		return rdma_alloc_hw_stats_struct(
			devcntr_names,
			num_dev_cntrs,
			RDMA_HW_STATS_DEFAULT_LIFESPAN);
	else
		return rdma_alloc_hw_stats_struct(
			portcntr_names,
			num_port_cntrs,
			RDMA_HW_STATS_DEFAULT_LIFESPAN);
}

static int hfi2_get_hw_stats(struct ib_device *ibdev,
			     struct rdma_hw_stats *stats, u8 port, int index)
{
	u64 *values;
	int count;

	if (!port) {
		hfi_read_devcntrs(hfi_dd_from_ibdev(ibdev), &values);
		count = num_dev_cntrs;
	} else {
		struct hfi2_ibport *ibp = to_hfi_ibp(ibdev, port);

		hfi_read_portcntrs(ibp->ppd, &values);
		count = num_port_cntrs;
	}

	memcpy(stats->value, values, count * sizeof(*values));
	return count;
}

static int hfi2_reg_mr(struct rvt_dev_info *rdi, struct rvt_mregion *mr)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(&rdi->ibdev);
	void *vaddr;
	int m, n, i, list_len;
	u32 ps;
	bool writable;

	/* TODO: only do the following operation in SPT mode */
	writable = !!(mr->access_flags &
		(IB_ACCESS_LOCAL_WRITE   | IB_ACCESS_REMOTE_WRITE |
		 IB_ACCESS_REMOTE_ATOMIC | IB_ACCESS_MW_BIND));

	ps = 1 << mr->page_shift;
	list_len = mr->length / ps;
	m = 0;
	n = 0;
	for (i = 0; i < list_len; i++) {
		vaddr = mr->map[m]->segs[n].vaddr;
		hfi_at_reg_range(&dd->priv_ctx, vaddr, ps, NULL, writable);
		if (++n == RVT_SEGSZ) {
			m++;
			n = 0;
		}
	}

	return 0;
}

static void hfi2_dereg_mr(struct rvt_dev_info *rdi, struct rvt_mregion *mr)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(&rdi->ibdev);
	void *vaddr;
	int m, n, i, list_len;
	u32 ps;

	/* TODO: only do the following operation in SPT mode */
	ps = 1 << mr->page_shift;
	list_len = mr->length / ps;
	m = 0;
	n = 0;
	for (i = 0; i < list_len; i++) {
		vaddr = mr->map[m]->segs[n].vaddr;
		hfi_at_dereg_range(&dd->priv_ctx, vaddr, ps);
		if (++n == RVT_SEGSZ) {
			m++;
			n = 0;
		}
	}
}

static int hfi2_register_device(struct hfi2_ibdev *ibd, const char *name)
{
	struct ib_device *ibdev = &ibd->rdi.ibdev;
	struct hfi2_ibport *ibp;
	int i, ret;

#if IS_ENABLED(CONFIG_INFINIBAND_EXP_USER_ACCESS)
	const struct uverbs_object_tree_def *hfi2_root[] = {
		uverbs_default_get_objects(),
		hfi2_get_objects()
	};
#endif
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
	ibdev->dev.parent = ibd->parent_dev;

	ibdev->alloc_ucontext = hfi2_alloc_ucontext;
	ibdev->dealloc_ucontext = hfi2_dealloc_ucontext;
	ibdev->query_pkey = hfi2_query_pkey;
	ibdev->query_gid = hfi2_query_gid;
	ibdev->process_mad = hfi2_process_mad;
	ibdev->modify_port = hfi2_modify_port;
	ibdev->modify_device = hfi2_modify_device;
	ibdev->get_hw_stats = hfi2_get_hw_stats;
	ibdev->alloc_hw_stats = hfi2_alloc_hw_stats;
	ibdev->alloc_rdma_netdev = hfi2_vnic_alloc_rn;

	/* add optional sysfs files under /sys/class/infiniband/hfi2_0/ */
	hfi2_verbs_register_sysfs(&ibdev->dev);

	/*
	 * Fill in rvt info object.
	 */
	ibd->rdi.driver_f.port_callback = port_callback;
	ibd->rdi.driver_f.get_pci_dev = get_pci_dev;

	ibd->rdi.driver_f.query_port_state = hfi2_query_port;
	ibd->rdi.driver_f.mmap = hfi2_mmap;
#if 0
	/* for query_guid, query_port, and modify_port */
	ibd->rdi.driver_f.get_guid_be = hfi1_get_guid_be;
	ibd->rdi.driver_f.shut_down_port = shut_down_port;
	ibd->rdi.driver_f.cap_mask_chg = hfi1_cap_mask_chg;
#endif

	/*
	 * Fill in rvt info device attributes.
	 */
	fill_device_attr(ibd);

	/* general parameters */
	ibd->rdi.dparms.lkey_table_size = hfi2_lkey_table_size;
	ibd->rdi.dparms.max_rdma_atomic = HFI2_MAX_RDMA_ATOMIC;
	ibd->rdi.dparms.psn_mask = PSN_MASK;
	ibd->rdi.dparms.psn_shift = PSN_SHIFT;
	ibd->rdi.dparms.psn_modify_mask = PSN_MODIFY_MASK;
	ibd->rdi.dparms.core_cap_flags = RDMA_CORE_PORT_INTEL_OPA |
					 RDMA_CORE_CAP_OPA_AH;
	ibd->rdi.dparms.max_mad_size = OPA_MGMT_MAD_SIZE;

	/* queue pair parameters */
	ibd->rdi.dparms.qp_table_size = hfi2_qp_table_size;
	ibd->rdi.dparms.qpn_start = 0;
	ibd->rdi.dparms.qpn_inc = 1;
	ibd->rdi.dparms.qos_shift = 1;
	ibd->rdi.dparms.qpn_res_start = HFI2_QPN_KDETH_BASE;
	ibd->rdi.dparms.qpn_res_end = ibd->rdi.dparms.qpn_res_start +
				      HFI2_QPN_KDETH_SIZE;

	/* basic queue pair support */
	ibd->rdi.driver_f.qp_priv_alloc = qp_priv_alloc;
	ibd->rdi.driver_f.qp_priv_free = qp_priv_free;
	ibd->rdi.driver_f.free_all_qps = free_all_qps;
	ibd->rdi.driver_f.notify_qp_reset = notify_qp_reset;
	ibd->rdi.driver_f.flush_qp_waiters = flush_qp_waiters;
	ibd->rdi.driver_f.stop_send_queue = stop_send_queue;
	ibd->rdi.driver_f.quiesce_qp = quiesce_qp;
	ibd->rdi.driver_f.notify_error_qp = notify_error_qp;
	ibd->rdi.driver_f.schedule_send = hfi2_schedule_send;
	/* modify QP support */
	ibd->rdi.driver_f.get_pmtu_from_attr = get_pmtu_from_attr;
	ibd->rdi.driver_f.mtu_from_qp = mtu_from_qp;
	ibd->rdi.driver_f.mtu_to_path_mtu = mtu_to_path_mtu;
	ibd->rdi.driver_f.check_modify_qp = hfi2_check_modify_qp;
	ibd->rdi.driver_f.modify_qp = hfi2_modify_qp;
	ibd->rdi.driver_f.notify_restart_rc = hfi2_restart_rc;
	/* for AH support */
	ibd->rdi.driver_f.check_ah = hfi2_check_ah;
	ibd->rdi.driver_f.notify_new_ah = hfi2_notify_new_ah;
	/* for post_send */
	ibd->rdi.driver_f.schedule_send_no_lock = hfi2_schedule_send_no_lock;
	ibd->rdi.driver_f.do_send = hfi2_do_send_from_rvt;
	ibd->rdi.driver_f.check_send_wqe = hfi2_check_send_wqe;

	/* maintain shadow page table during mr management */
	ibd->rdi.driver_f.reg_mr = hfi2_reg_mr;
	ibd->rdi.driver_f.dereg_mr = hfi2_dereg_mr;

	/* post send table */
	ibd->rdi.post_parms = hfi2_post_parms;

	/* completion queue */
	snprintf(ibd->rdi.dparms.cq_name, sizeof(ibd->rdi.dparms.cq_name),
		 "hfi2_cq%d", ibd->dd->unit);
	rvt_set_ibdev_name(&ibd->rdi, "%s_%d", hfi_class_name(), ibd->dd->unit);
	ibd->rdi.dparms.node = ibd->assigned_node_id;

	ibd->rdi.dparms.nports = ibd->num_pports;
	ibd->rdi.dparms.npkeys = HFI_MAX_PKEYS;

	ibp = ibd->pport;
	for (i = 0; i < ibd->num_pports; i++, ibp++)
		rvt_init_port(&ibd->rdi, &ibp->rvp, i, ibp->ppd->pkeys);

#if IS_ENABLED(CONFIG_INFINIBAND_EXP_USER_ACCESS)
	/* Extended verbs root */
	ibd->rdi.ibdev.specs_root = uverbs_alloc_spec_tree(2, hfi2_root);
#endif
	ret = rvt_register_device(&ibd->rdi, RDMA_DRIVER_HFI2);
	if (ret)
		goto err_reg;
	return ret;

err_reg:
	dd_dev_err(ibd->dd, "Failed to register with RDMAVT: %d\n", ret);
	return ret;
}

static void hfi2_unregister_device(struct hfi2_ibdev *ibd)
{
	if (ibd->rdi.ibdev.specs_root)
		uverbs_free_spec_tree(ibd->rdi.ibdev.specs_root);

	rvt_unregister_device(&ibd->rdi);
}

static int hfi2_init_port(struct hfi2_ibdev *ibd,
			  struct hfi_pportdata *ppd)
{
	int ret, i;
	int pidx = ppd->pnum - 1;
	struct hfi2_ibport *ibp = &ibd->pport[pidx];

	for (i = 0; i < RVT_MAX_TRAP_LISTS; i++)
		INIT_LIST_HEAD(&ibp->rvp.trap_lists[i].list);
	timer_setup(&ibp->rvp.trap_timer, hfi_handle_trap_timer, 0);

	ibp->ibd = ibd;
	ibp->dev = &ibd->rdi.ibdev.dev; /* for dev_info, etc. */
	ibp->ppd = ppd;
	ibp->stats = &ibd->dd->stats;
	ibp->rvp.gid_prefix = IB_DEFAULT_GID_PREFIX;
	ibp->rvp.sm_lid = 0;

	ibp->rvp.rc_acks = alloc_percpu(u64);
	ibp->rvp.rc_qacks = alloc_percpu(u64);
	ibp->rvp.rc_delayed_comp = alloc_percpu(u64);
	if (!ibp->rvp.rc_acks || !ibp->rvp.rc_qacks ||
	    !ibp->rvp.rc_delayed_comp) {
		/* error path does any needed free_percpu() */
		ret = -ENOMEM;
		goto err;
	}

	/* Below should only set bits defined in OPA PortInfo.CapabilityMask */
	ibp->rvp.port_cap_flags = IB_PORT_AUTO_MIGR_SUP |
				  IB_PORT_CAP_MASK_NOTICE_SUP;
	ibp->rvp.port_cap3_flags = OPA_CAP_MASK3_IsSharedSpaceSupported |
				   OPA_CAP_MASK3_IsVLrSupported |
				   OPA_CAP_MASK3_IsMAXLIDSupported |
				   OPA_CAP_MASK3_IsBwMeterSupported |
				   OPA_CAP_MASK3_IsVLMarkerSupported |
				   OPA_CAP_MASK3_IsSLPairsSupported |
				   OPA_CAP_MASK3_IsTimeSyncSupported;

	ibp->port_num = ppd->pnum - 1;

	/* FXRTODO - this should all be inside rvt_init_port */
	RCU_INIT_POINTER(ibp->rvp.qp[0], NULL);
	RCU_INIT_POINTER(ibp->rvp.qp[1], NULL);
	spin_lock_init(&ibp->rvp.lock);

	ret = hfi_pend_cq_info_alloc(ibp->ibd->dd, &ibp->pend_cq);
	if (ret < 0)
		goto err;

	ret = hfi2_ctx_init_port(ibp);
	if (ret < 0)
		goto err;

	ibp->prescan_9B = false;
	ibp->prescan_16B = false;
	/* start RX processing, call this last after no errors */
	hfi2_ctx_start_port(ibp);
	return 0;

err:
	hfi2_uninit_port(ibp);
	return ret;
}

static void hfi2_uninit_port(struct hfi2_ibport *ibp)
{
	hfi2_ctx_uninit_port(ibp);
	hfi_pend_cq_info_free(&ibp->pend_cq);
	free_percpu(ibp->rvp.rc_acks);
	free_percpu(ibp->rvp.rc_qacks);
	free_percpu(ibp->rvp.rc_delayed_comp);
}

int hfi2_ib_add(struct hfi_devdata *dd)
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
	ibd->parent_dev = &dd->pdev->dev;
	ibd->rc_drop_enabled = rc_drop_enabled;
	ibd->rc_drop_max_rate = HFI_MAX_DROP_RATE;

	/* set RHF event table for Verbs receive */
	ibd->rhf_rcv_functions[RHF_RCV_TYPE_IB] = hfi2_ib_rcv;
	ibd->rhf_rcv_functions[RHF_RCV_TYPE_BYPASS] = hfi2_ib_rcv;
	ibd->rhf_rcv_function_map = ibd->rhf_rcv_functions;
	/* Verbs send functions, can be replaced by snoop */
	ibd->send_wqe = hfi2_send_wqe;
	ibd->send_ack = hfi2_send_ack;

	/* Allocate HFI Contexts */
	ret = hfi2_ctx_init(ibd, HFI2_IB_DEF_CTXTS);
	if (ret)
		goto ctx_err;

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

	ret = wqe_cache_create(ibd);
	if (ret)
		goto ib_reg_err;

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
	rvt_dealloc_device(&ibd->rdi);
exit:
	/* must clear dd->ibd so state is sane during error cleanup */
	dd->ibd = NULL;
	dev_err(&dd->pdev->dev, "%s error rc %d\n", __func__, ret);
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
	rvt_dealloc_device(&ibd->rdi);
	wqe_cache_destroy(ibd);
}

void hfi_verbs_dbg_init(struct hfi_devdata *dd)
{
	debugfs_create_bool("rc_drop_enabled", 0644,
			    dd->hfi_dev_dbg, &dd->ibd->rc_drop_enabled);
	debugfs_create_u8("rc_drop_max_rate", 0644,
			  dd->hfi_dev_dbg, &dd->ibd->rc_drop_max_rate);
}
