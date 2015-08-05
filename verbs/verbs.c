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
 * Intel(R) OPA Gen2 IB Driver
 */

#include <rdma/ib_mad.h>
#include <rdma/ib_user_verbs.h>
#include <linux/idr.h>
#include <linux/module.h>
#include <linux/utsname.h>
#include <linux/pci.h>
#include "mad.h"
#include "verbs.h"
#include <rdma/opa_core_ib.h>

/* FXRTODO - need definition of eager buffer */
#define OPA2_IB_ME_COUNT 1024

static int opa_ib_add(struct opa_core_device *odev);
static void opa_ib_remove(struct opa_core_device *odev);

static struct opa_core_client opa_ib_driver = {
	.name = KBUILD_MODNAME,
	.add = opa_ib_add,
	.remove = opa_ib_remove,
};

/* TODO - placeholders */
__be64 opa_ib_sys_guid;

/* TODO - to be used in various alloc routines */
/* Maximum number of protection domains to support */
unsigned int opa_ib_max_pds = 0xFFFF;
/* Maximum number of address handles to support */
unsigned int opa_ib_max_ahs = 0xFFFF;
/* Maximum number of completion queue entries to support */
unsigned int opa_ib_max_cqes = 0x2FFFF;
/* Maximum number of completion queues to support */
unsigned int opa_ib_max_cqs = 0x1FFFF;
/* Maximum number of QP WRs to support */
unsigned int opa_ib_max_qp_wrs = 0x3FFF;
/* Maximum number of QPs to support */
unsigned int opa_ib_max_qps = 16384;
/* Maximum number of SGEs to support */
unsigned int opa_ib_max_sges = 0x60;
/* Maximum number of multicast groups to support */
unsigned int opa_ib_max_mcast_grps = 16384;
/* Maximum number of attached QPs to support */
unsigned int opa_ib_max_mcast_qp_attached = 16;
/* Maximum number of SRQs to support */
unsigned int opa_ib_max_srqs = 1024;
/* Maximum number of SRQ SGEs to support */
unsigned int opa_ib_max_srq_sges = 128;
/* Maximum number of SRQ WRs support */
unsigned int opa_ib_max_srq_wrs = 0x1FFFF;
/* LKEY table size in bits (2^n, 1 <= n <= 23) */
unsigned int opa_ib_lkey_table_size = 16;

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

static int opa_ib_query_device(struct ib_device *ibdev,
			       struct ib_device_attr *props)
{
	struct opa_ib_data *ibd = to_opa_ibdata(ibdev);
	struct opa_ib_portdata *ibp1 = to_opa_ibportdata(ibdev, 1);

	memset(props, 0, sizeof(*props));

	/* TODO - first cut at device attributes */

	props->device_cap_flags = IB_DEVICE_BAD_PKEY_CNTR |
		IB_DEVICE_BAD_QKEY_CNTR | IB_DEVICE_SHUTDOWN_PORT |
		IB_DEVICE_SYS_IMAGE_GUID | IB_DEVICE_RC_RNR_NAK_GEN |
		IB_DEVICE_PORT_ACTIVE_EVENT | IB_DEVICE_SRQ_RESIZE |
		IB_DEVICE_JUMBO_MAD_SUPPORT;

	props->page_size_cap = PAGE_SIZE;
#if 0
	props->vendor_id = id.vendor;
		dd->oui1 << 16 | dd->oui2 << 8 | dd->oui3;
	props->vendor_part_id = dd->pcidev->device;
	props->hw_ver = dd->minrev;
#else
	props->vendor_id = ibd->odev->id.vendor;
	props->vendor_part_id = ibd->odev->id.device;
	props->hw_ver = 0;
#endif
	props->sys_image_guid = opa_ib_sys_guid;
	props->max_mr_size = ~0ULL;
	props->max_qp = opa_ib_max_qps;
	props->max_qp_wr = opa_ib_max_qp_wrs;
	props->max_sge = opa_ib_max_sges;
	props->max_cq = opa_ib_max_cqs;
	props->max_ah = opa_ib_max_ahs;
	props->max_cqe = opa_ib_max_cqes;
	props->max_mr = 1 << opa_ib_lkey_table_size;
	props->max_fmr = 1 << opa_ib_lkey_table_size;
	props->max_map_per_fmr = OPA_IB_MAX_MAP_PER_FMR;
	props->max_pd = opa_ib_max_pds;
	props->max_qp_rd_atom = OPA_IB_MAX_RDMA_ATOMIC;
	props->max_qp_init_rd_atom = OPA_IB_INIT_RDMA_ATOMIC;
	/* props->max_res_rd_atom */
	props->max_srq = opa_ib_max_srqs;
	props->max_srq_wr = opa_ib_max_srq_wrs;
	props->max_srq_sge = opa_ib_max_srq_sges;
	/* props->local_ca_ack_delay */
	props->atomic_cap = IB_ATOMIC_GLOB;
	props->max_pkeys = ibp1->pkey_tlen;
	props->max_mcast_grp = opa_ib_max_mcast_grps;
	props->max_mcast_qp_attach = opa_ib_max_mcast_qp_attached;
	props->max_total_mcast_qp_attach = props->max_mcast_qp_attach *
					   props->max_mcast_grp;

	return 0;
}

static int opa_ib_query_port(struct ib_device *ibdev, u8 port,
			     struct ib_port_attr *props)
{
	struct opa_ib_portdata *ibp = to_opa_ibportdata(ibdev, port);

	if (!ibp)
		return -EINVAL;

	memset(props, 0, sizeof(*props));

	/* TODO - first cut at port attributes */

	props->lid = ibp->lid;
	props->lmc = ibp->lmc;
	props->sm_lid = ibp->sm_lid;
	props->sm_sl = ibp->smsl;
#if 0
	//props->state = dd->f_iblink_state(ppd);
	//props->phys_state = dd->f_ibphys_portstate(ppd);
#else
	/* FXRTODO: Implement linkup */
	props->state = IB_PORT_ACTIVE;
	props->phys_state = IB_PORTPHYSSTATE_POLLING;
#endif
	props->port_cap_flags = ibp->port_cap_flags;
	props->gid_tbl_len = 1;
	props->max_msg_sz = OPA_IB_MAX_MSG_SZ;
	props->pkey_tbl_len = ibp->pkey_tlen;
	props->bad_pkey_cntr = ibp->pkey_violations;
	props->qkey_viol_cntr = ibp->qkey_violations;
	props->active_width = (u8)opa_width_to_ib(ibp->link_width_active);
	props->active_speed = (u8)opa_speed_to_ib(ibp->link_speed_active);
	props->max_vl_num = ibp->max_vls;
	props->init_type_reply = 0;

	/* Once we are a "first class" citizen and have added the OPA MTUs to
	 * the core we can advertise the larger MTU enum to the ULPs, for now
	 * advertise only 4K.
	 *
	 * Those applications which are either OPA aware or pass the MTU enum
	 * from the Path Records to us will get the new 8k MTU.  Those that
	 * attempt to process the MTU enum may fail in various ways.
	 */
	props->max_mtu = opa_mtu_to_enum_safe((valid_ib_mtu(ibp->ibmaxmtu) ?
				      ibp->ibmaxmtu : 4096), IB_MTU_4096);
	props->active_mtu = valid_ib_mtu(ibp->ibmtu) ?
		opa_mtu_to_enum_safe(ibp->ibmtu, IB_MTU_2048) : props->max_mtu;
	props->subnet_timeout = ibp->subnet_timeout;

	return 0;
}

static int opa_ib_query_pkey(struct ib_device *ibdev, u8 port, u16 index,
			  u16 *pkey)
{
	struct opa_ib_portdata *ibp = to_opa_ibportdata(ibdev, port);

	if (!ibp || index >= ibp->pkey_tlen)
		return -EINVAL;

	*pkey = ibp->pkeys[index];
	return 0;
}

static int opa_ib_query_gid(struct ib_device *ibdev, u8 port,
			 int index, union ib_gid *gid)
{
	struct opa_ib_portdata *ibp = to_opa_ibportdata(ibdev, port);
	int ret = 0;

	if (!ibp || index != 0)
		return -EINVAL;

	gid->global.subnet_prefix = ibp->gid_prefix;
	gid->global.interface_id = ibp->guid;
	return ret;
}

/**
 * qib_alloc_ucontext - allocate a ucontext
 * @ibdev: the infiniband device
 * @udata: not used
 */

static struct ib_ucontext *opa_ib_alloc_ucontext(struct ib_device *ibdev,
						struct ib_udata *udata)
{
	struct opa_ucontext *context;
	struct ib_ucontext *ret;

	context = kmalloc(sizeof(*context), GFP_KERNEL);
	if (!context) {
		ret = ERR_PTR(-ENOMEM);
		goto bail;
	}

	ret = &context->ibucontext;

bail:
	return ret;
}

/**
 * opa_ib_dealloc_ucontext - deallocate a ucontext
 * @context: the context to deallocate
 */

static int opa_ib_dealloc_ucontext(struct ib_ucontext *context)
{
	kfree(to_opa_ucontext(context));
	return 0;
}

static int opa_ib_register_device(struct opa_ib_data *ibd, const char *name)
{
	struct ib_device *ibdev = &ibd->ibdev;
	int ret;
	size_t lcpysz = IB_DEVICE_NAME_MAX;

	lcpysz = strlcpy(ibdev->name, name, lcpysz);
	strlcpy(ibdev->name + lcpysz, "%d", lcpysz);
	strncpy(ibdev->node_desc, init_utsname()->nodename,
		sizeof(ibdev->node_desc));
	ibdev->node_guid = ibd->node_guid;
	ibdev->node_type = RDMA_NODE_IB_CA;
	ibdev->num_comp_vectors = 1;
	ibdev->owner = THIS_MODULE;
	ibdev->phys_port_cnt = ibd->num_pports;
	ibdev->uverbs_abi_ver = IB_USER_VERBS_ABI_VERSION;
	ibdev->uverbs_cmd_mask =
		(1ull << IB_USER_VERBS_CMD_GET_CONTEXT)         |
		(1ull << IB_USER_VERBS_CMD_QUERY_DEVICE)        |
		(1ull << IB_USER_VERBS_CMD_QUERY_PORT)          |
		(1ull << IB_USER_VERBS_CMD_ALLOC_PD)            |
		(1ull << IB_USER_VERBS_CMD_DEALLOC_PD)          |
		(1ull << IB_USER_VERBS_CMD_CREATE_AH)           |
		(1ull << IB_USER_VERBS_CMD_MODIFY_AH)           |
		(1ull << IB_USER_VERBS_CMD_QUERY_AH)            |
		(1ull << IB_USER_VERBS_CMD_DESTROY_AH)          |
		(1ull << IB_USER_VERBS_CMD_REG_MR)              |
		(1ull << IB_USER_VERBS_CMD_DEREG_MR)            |
		(1ull << IB_USER_VERBS_CMD_CREATE_COMP_CHANNEL) |
		(1ull << IB_USER_VERBS_CMD_CREATE_CQ)           |
		(1ull << IB_USER_VERBS_CMD_RESIZE_CQ)           |
		(1ull << IB_USER_VERBS_CMD_DESTROY_CQ)          |
		(1ull << IB_USER_VERBS_CMD_POLL_CQ)             |
		(1ull << IB_USER_VERBS_CMD_REQ_NOTIFY_CQ)       |
		(1ull << IB_USER_VERBS_CMD_CREATE_QP)           |
		(1ull << IB_USER_VERBS_CMD_QUERY_QP)            |
		(1ull << IB_USER_VERBS_CMD_MODIFY_QP)           |
		(1ull << IB_USER_VERBS_CMD_DESTROY_QP)          |
		(1ull << IB_USER_VERBS_CMD_POST_SEND)           |
		(1ull << IB_USER_VERBS_CMD_POST_RECV);
	ibdev->query_device = opa_ib_query_device;
	ibdev->query_port = opa_ib_query_port;
	ibdev->query_pkey = opa_ib_query_pkey;
	ibdev->query_gid = opa_ib_query_gid;
	ibdev->alloc_pd = opa_ib_alloc_pd;
	ibdev->dealloc_pd = opa_ib_dealloc_pd;
	ibdev->create_ah = opa_ib_create_ah;
	ibdev->modify_ah = opa_ib_modify_ah;
	ibdev->query_ah = opa_ib_query_ah;
	ibdev->destroy_ah = opa_ib_destroy_ah;
	ibdev->create_qp = opa_ib_create_qp;
	ibdev->modify_qp = opa_ib_modify_qp;
	ibdev->query_qp = opa_ib_query_qp;
	ibdev->destroy_qp = opa_ib_destroy_qp;
	ibdev->post_send = opa_ib_post_send;
	ibdev->post_recv = opa_ib_post_receive;
	ibdev->create_cq = opa_ib_create_cq;
	ibdev->destroy_cq = opa_ib_destroy_cq;
	ibdev->resize_cq = opa_ib_resize_cq;
	ibdev->poll_cq = opa_ib_poll_cq;
	ibdev->req_notify_cq = opa_ib_req_notify_cq;
	ibdev->get_dma_mr = opa_ib_get_dma_mr;
	ibdev->dereg_mr = opa_ib_dereg_mr;
	ibdev->process_mad = opa_ib_process_mad;
	ibdev->alloc_ucontext = opa_ib_alloc_ucontext;
	ibdev->dealloc_ucontext = opa_ib_dealloc_ucontext;
	ibdev->dma_device = ibd->parent_dev;
#if 0
	ibdev->modify_device = opa_ib_modify_device;
	ibdev->modify_port = opa_ib_modify_port;
	ibdev->create_srq = opa_ib_create_srq;
	ibdev->modify_srq = opa_ib_modify_srq;
	ibdev->query_srq = opa_ib_query_srq;
	ibdev->destroy_srq = opa_ib_destroy_srq;
	ibdev->post_srq_recv = opa_ib_post_srq_receive;
	ibdev->reg_phys_mr = opa_ib_reg_phys_mr;
	ibdev->reg_user_mr = opa_ib_reg_user_mr;
	ibdev->alloc_fast_reg_mr = opa_ib_alloc_fast_reg_mr;
	ibdev->alloc_fast_reg_page_list = opa_ib_alloc_fast_reg_page_list;
	ibdev->free_fast_reg_page_list = opa_ib_free_fast_reg_page_list;
	ibdev->alloc_fmr = opa_ib_alloc_fmr;
	ibdev->map_phys_fmr = opa_ib_map_phys_fmr;
	ibdev->unmap_fmr = opa_ib_unmap_fmr;
	ibdev->dealloc_fmr = opa_ib_dealloc_fmr;
	ibdev->attach_mcast = opa_ib_multicast_attach;
	ibdev->detach_mcast = opa_ib_multicast_detach;
#endif
	ibdev->mmap = opa_ib_mmap;
	ibdev->dma_ops = &opa_ib_dma_mapping_ops;

	ret = ib_register_device(ibdev, NULL); //opa_ib_create_port_files);
	if (ret)
		goto err_reg;

#if 0
	ret = opa_ib_create_agents(dev);
	if (ret)
		goto err_agents;

	if (opa_ib_verbs_register_sysfs(dd))
		goto err_class;
#endif
	goto exit;

#if 0
err_class:
	opa_ib_free_agents(dev);
err_agents:
	ib_unregister_device(ibdev);
#endif
err_reg:
	pr_err("Failed to register with IB core: %d\n", ret);
exit:
	return ret;
}

static void opa_ib_unregister_device(struct opa_ib_data *ibd)
{
	ib_unregister_device(&ibd->ibdev);
}

static int opa_ib_hw_init(struct opa_ib_portdata *ibp)
{
	struct hfi_ctx *ctx = &ibp->ctx;
	struct opa_ctx_assign ctx_assign = {0};
	struct opa_core_ops *ops = ibp->odev->bus_ops;
	u16 cq_idx;
	int rc;

	HFI_CTX_INIT_BYPASS(ctx, ibp->odev->dd);
	ctx_assign.pid = HFI_PID_ANY;
	ctx_assign.le_me_count = OPA2_IB_ME_COUNT;
	rc = ops->ctx_assign(ctx, &ctx_assign);
	if (rc)
		return rc;

	/* Obtain a pair of command queues and setup */
	rc = ops->cq_assign(ctx, NULL, &cq_idx);
	if (rc)
		goto err;
	rc = ops->cq_map(ctx, cq_idx, &ibp->cmdq_tx, &ibp->cmdq_rx);
	if (rc)
		goto err1;

	return 0;
err1:
	ops->cq_release(ctx, cq_idx);
err:
	ops->ctx_release(ctx);
	return rc;
}

static void opa_ib_hw_uninit(struct opa_ib_portdata *ibp)
{
	struct hfi_ctx *ctx = &ibp->ctx;
	struct opa_core_ops *ops = ibp->odev->bus_ops;
	u16 cq_idx = ibp->cmdq_tx.cq_idx;

	ops->cq_unmap(&ibp->cmdq_tx, &ibp->cmdq_rx);
	ops->cq_release(ctx, cq_idx);
	ops->ctx_release(ctx);
}

static int opa_ib_init_port(struct opa_ib_data *ibd,
			struct opa_core_device *odev, u8 pidx)
{
	int i, ret;
	struct opa_ib_portdata *ibp = &ibd->pport[pidx];
	struct opa_pport_desc pdesc;
	struct opa_core_ops *ops = odev->bus_ops;

	ops->get_port_desc(odev, &pdesc, pidx + 1);

	ibp->odev = odev;
	ibp->gid_prefix = IB_DEFAULT_GID_PREFIX;
	ibp->guid = pdesc.pguid;
	ibp->ibmaxmtu = pdesc.ibmaxmtu;
	ibp->max_vls = pdesc.num_vls_supported;
	ibp->lid = pdesc.lid;
	ibp->sm_lid = 0;
	/*
	 * FXRTODO: These need to be reset to their
	 * defaults after every linkup also. Once LNI code is up
	 * and working. we will need a notification to the
	 * verbs layer on linkup event.
	 */
	ibp->sm_trap_qp = OPA_DEFAULT_SM_TRAP_QP;
	ibp->sa_qp = OPA_DEFAULT_SA_QP;
	ibp->link_width_active = OPA_LINK_WIDTH_4X;
	/* FXRTODO - this should be not yet defined OPA_LINK_SPEED_32G */
	ibp->link_speed_active = OPA_LINK_SPEED_25G;
	/* Below should only set bits defined in OPA PortInfo.CapabilityMask */
	ibp->port_cap_flags = IB_PORT_AUTO_MIGR_SUP |
		IB_PORT_CAP_MASK_NOTICE_SUP;
	ibp->pkey_tlen = pdesc.pkey_tlen;
	ibp->pkeys = pdesc.pkeys;

	RCU_INIT_POINTER(ibp->qp0, NULL);
	RCU_INIT_POINTER(ibp->qp1, NULL);

	/* MTU is per-VL */
	for (i = 0; i < ibp->max_vls; i++)
		ibp->vl_mtu[i] = pdesc.vl_mtu[i];
	ibp->vl_mtu[15] = pdesc.vl_mtu[15];

	for (i = 0; i < ARRAY_SIZE(ibp->sl_to_sc); i++)
		ibp->sl_to_sc[i] = pdesc.sl_to_sc[i];
	for (i = 0; i < ARRAY_SIZE(ibp->sc_to_sl); i++)
		ibp->sc_to_sl[i] = pdesc.sc_to_sl[i];
	/*
	 * FXRTODO: quick hack at initial SC to VL tables.
	 * Likely this needs to be retrieved from opa2_hfi.
	 */
	for (i = 0; i < ARRAY_SIZE(ibp->sc_to_vl); i++)
		ibp->sc_to_vl[i] = i % ibp->max_vls;
	/* management SC15 always uses VL15 */
	ibp->sc_to_vl[15] = 15;

	/* FXRTODO: assign Portals PID, currently one per port */
	ret = opa_ib_hw_init(ibp);

	return ret;
}

static void opa_ib_uninit_port(struct opa_ib_portdata *ibp)
{
	opa_ib_hw_uninit(ibp);
}

static int opa_ib_add(struct opa_core_device *odev)
{
	int i, ret;
	u8 num_ports;
	struct opa_ib_data *ibd;
	struct opa_ib_portdata *ibp;
	struct opa_dev_desc desc;
	struct opa_core_ops *ops = odev->bus_ops;

	ops->get_device_desc(odev, &desc);
	num_ports = desc.num_pports;
	ibd = kzalloc(sizeof(*ibd) + sizeof(*ibp) * num_ports, GFP_KERNEL);
	if (!ibd) {
		ret = -ENOMEM;
		goto exit;
	}

	ibd->num_pports = num_ports;
	ibd->node_guid = desc.nguid;
	memcpy(ibd->oui, desc.oui, ARRAY_SIZE(ibd->oui));
	ibd->assigned_node_id = desc.numa_node;
	ibd->pport = (struct opa_ib_portdata *)(ibd + 1);
	ibd->parent_dev = odev->dev.parent;
	ibd->odev = odev;

	ida_init(&ibd->qpn_table);
	spin_lock_init(&ibd->qpt_lock);
	INIT_LIST_HEAD(&ibd->pending_mmaps);
	spin_lock_init(&ibd->pending_lock);
	ibd->mmap_offset = PAGE_SIZE;
	spin_lock_init(&ibd->mmap_offset_lock);
	spin_lock_init(&ibd->n_pds_lock);
	spin_lock_init(&ibd->n_ahs_lock);
	spin_lock_init(&ibd->n_cqs_lock);
	spin_lock_init(&ibd->n_qps_lock);
	spin_lock_init(&ibd->n_srqs_lock);

	ret = opa_ib_cq_init(ibd);
	if (ret)
		goto cq_init_err;

	/* FXRTODO: Move pkey support to opa2_hfi  */
	for (i = 0; i < num_ports; i++) {
		ret = opa_ib_init_port(ibd, odev, i);
		if (ret) {
			while (--i >= 0)
				opa_ib_uninit_port(&ibd->pport[i]);
			break;
		}
	}
	if (ret)
		goto port_err;

	ret = opa_core_set_priv_data(&opa_ib_driver, odev, ibd);
	if (ret)
		goto priv_err;
	ret = opa_ib_register_device(ibd, opa_ib_driver.name);
	if (ret)
		goto ib_reg_err;
	return ret;
ib_reg_err:
	opa_core_clear_priv_data(&opa_ib_driver, odev);
priv_err:
	for (i = 0; i < num_ports; i++)
		opa_ib_uninit_port(&ibd->pport[i]);
port_err:
	opa_ib_cq_exit(ibd);
cq_init_err:
	kfree(ibd);
exit:
	dev_err(&odev->dev, "%s error rc %d\n", __func__, ret);
	return ret;
}

static void opa_ib_remove(struct opa_core_device *odev)
{
	int i;
	struct opa_ib_data *ibd;

	ibd = opa_core_get_priv_data(&opa_ib_driver, odev);
	if (!ibd)
		return;
	opa_ib_unregister_device(ibd);
	for (i = 0; i < ibd->num_pports; i++)
		opa_ib_uninit_port(&ibd->pport[i]);
	opa_ib_cq_exit(ibd);
	ida_destroy(&ibd->qpn_table);
	kfree(ibd);
	opa_core_clear_priv_data(&opa_ib_driver, odev);
}

int __init opa_ib_init(void)
{
	return opa_core_client_register(&opa_ib_driver);
}
module_init(opa_ib_init);

void opa_ib_cleanup(void)
{
	opa_core_client_unregister(&opa_ib_driver);
}
module_exit(opa_ib_cleanup);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel(R) OPA Gen2 IB Driver");
