// SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
/* Copyright(c) 2016-2018 Intel Corporation */

#include "dlb_bitmap.h"
#include "dlb_hw_types.h"
#include "dlb_regs.h"
#include "dlb_resource.h"

static void dlb_init_fn_rsrc_lists(struct dlb_function_resources *rsrc)
{
	INIT_LIST_HEAD(&rsrc->avail_domains);
	INIT_LIST_HEAD(&rsrc->used_domains);
	INIT_LIST_HEAD(&rsrc->avail_ldb_queues);
	INIT_LIST_HEAD(&rsrc->avail_ldb_ports);
	INIT_LIST_HEAD(&rsrc->avail_dir_pq_pairs);
	INIT_LIST_HEAD(&rsrc->avail_ldb_credit_pools);
	INIT_LIST_HEAD(&rsrc->avail_dir_credit_pools);
}

static void dlb_init_domain_rsrc_lists(struct dlb_domain *domain)
{
	INIT_LIST_HEAD(&domain->used_ldb_queues);
	INIT_LIST_HEAD(&domain->used_ldb_ports);
	INIT_LIST_HEAD(&domain->used_dir_pq_pairs);
	INIT_LIST_HEAD(&domain->used_ldb_credit_pools);
	INIT_LIST_HEAD(&domain->used_dir_credit_pools);
	INIT_LIST_HEAD(&domain->avail_ldb_queues);
	INIT_LIST_HEAD(&domain->avail_ldb_ports);
	INIT_LIST_HEAD(&domain->avail_dir_pq_pairs);
	INIT_LIST_HEAD(&domain->avail_ldb_credit_pools);
	INIT_LIST_HEAD(&domain->avail_dir_credit_pools);
}

static void dlb_resource_bitmap_free(struct dlb_function_resources *rsrcs)
{
	if (rsrcs->avail_hist_list_entries)
		dlb_bitmap_free(rsrcs->avail_hist_list_entries);

	if (rsrcs->avail_qed_freelist_entries)
		dlb_bitmap_free(rsrcs->avail_qed_freelist_entries);

	if (rsrcs->avail_dqed_freelist_entries)
		dlb_bitmap_free(rsrcs->avail_dqed_freelist_entries);

	if (rsrcs->avail_aqed_freelist_entries)
		dlb_bitmap_free(rsrcs->avail_aqed_freelist_entries);
}

void dlb_resource_free(struct dlb_hw *hw)
{
	int i;

	dlb_resource_bitmap_free(&hw->pf);

	for (i = 0; i < DLB_MAX_NUM_VFS; i++)
		dlb_resource_bitmap_free(&hw->vf[i]);
}

int dlb_resource_init(struct dlb_hw *hw)
{
	struct list_head *list;
	unsigned int i;
	int ret;

	/* For optimal load-balancing, ports that map to one or more QIDs in
	 * common should not be in numerical sequence. This is application
	 * dependent, but the driver interleaves port IDs as much as possible
	 * to reduce the likelihood of this. This initial allocation maximizes
	 * the average distance between an ID and its immediate neighbors (i.e.
	 * the distance from 1 to 0 and to 2, the distance from 2 to 1 and to
	 * 3, etc.).
	 */
	u32 init_ldb_port_allocation[DLB_MAX_NUM_LDB_PORTS] = {
		0,  31, 62, 29, 60, 27, 58, 25, 56, 23, 54, 21, 52, 19, 50, 17,
		48, 15, 46, 13, 44, 11, 42,  9, 40,  7, 38,  5, 36,  3, 34, 1,
		32, 63, 30, 61, 28, 59, 26, 57, 24, 55, 22, 53, 20, 51, 18, 49,
		16, 47, 14, 45, 12, 43, 10, 41,  8, 39,  6, 37,  4, 35,  2, 33
	};

	/* Zero-out resource tracking data structures */
	memset(&hw->rsrcs, 0, sizeof(hw->rsrcs));
	memset(&hw->pf, 0, sizeof(hw->pf));

	dlb_init_fn_rsrc_lists(&hw->pf);

	for (i = 0; i < DLB_MAX_NUM_VFS; i++) {
		memset(&hw->vf[i], 0, sizeof(hw->vf[i]));
		dlb_init_fn_rsrc_lists(&hw->vf[i]);
	}

	for (i = 0; i < DLB_MAX_NUM_DOMAINS; i++) {
		memset(&hw->domains[i], 0, sizeof(hw->domains[i]));
		dlb_init_domain_rsrc_lists(&hw->domains[i]);
		hw->domains[i].parent_func = &hw->pf;
	}

	/* Give all resources to the PF driver */
	hw->pf.num_avail_domains = DLB_MAX_NUM_DOMAINS;
	for (i = 0; i < hw->pf.num_avail_domains; i++) {
		list = &hw->domains[i].func_list;

		list_add(list, &hw->pf.avail_domains);
	}

	hw->pf.num_avail_ldb_queues = DLB_MAX_NUM_LDB_QUEUES;
	for (i = 0; i < hw->pf.num_avail_ldb_queues; i++) {
		list = &hw->rsrcs.ldb_queues[i].func_list;

		list_add(list, &hw->pf.avail_ldb_queues);
	}

	hw->pf.num_avail_ldb_ports = DLB_MAX_NUM_LDB_PORTS;
	for (i = 0; i < hw->pf.num_avail_ldb_ports; i++) {
		struct dlb_ldb_port *port;

		port = &hw->rsrcs.ldb_ports[init_ldb_port_allocation[i]];

		list_add(&port->func_list, &hw->pf.avail_ldb_ports);
	}

	hw->pf.num_avail_dir_pq_pairs = DLB_MAX_NUM_DIR_PORTS;
	for (i = 0; i < hw->pf.num_avail_dir_pq_pairs; i++) {
		list = &hw->rsrcs.dir_pq_pairs[i].func_list;

		list_add(list, &hw->pf.avail_dir_pq_pairs);
	}

	hw->pf.num_avail_ldb_credit_pools = DLB_MAX_NUM_LDB_CREDIT_POOLS;
	for (i = 0; i < hw->pf.num_avail_ldb_credit_pools; i++) {
		list = &hw->rsrcs.ldb_credit_pools[i].func_list;

		list_add(list, &hw->pf.avail_ldb_credit_pools);
	}

	hw->pf.num_avail_dir_credit_pools = DLB_MAX_NUM_DIR_CREDIT_POOLS;
	for (i = 0; i < hw->pf.num_avail_dir_credit_pools; i++) {
		list = &hw->rsrcs.dir_credit_pools[i].func_list;

		list_add(list, &hw->pf.avail_dir_credit_pools);
	}

	/* There are 5120 history list entries, which allows us to overprovision
	 * the inflight limit (4096) by 1k.
	 */
	ret = dlb_bitmap_alloc(&hw->pf.avail_hist_list_entries,
			       DLB_MAX_NUM_HIST_LIST_ENTRIES);
	if (ret)
		goto unwind;

	ret = dlb_bitmap_fill(hw->pf.avail_hist_list_entries);
	if (ret)
		goto unwind;

	ret = dlb_bitmap_alloc(&hw->pf.avail_qed_freelist_entries,
			       DLB_MAX_NUM_LDB_CREDITS);
	if (ret)
		goto unwind;

	ret = dlb_bitmap_fill(hw->pf.avail_qed_freelist_entries);
	if (ret)
		goto unwind;

	ret = dlb_bitmap_alloc(&hw->pf.avail_dqed_freelist_entries,
			       DLB_MAX_NUM_DIR_CREDITS);
	if (ret)
		goto unwind;

	ret = dlb_bitmap_fill(hw->pf.avail_dqed_freelist_entries);
	if (ret)
		goto unwind;

	ret = dlb_bitmap_alloc(&hw->pf.avail_aqed_freelist_entries,
			       DLB_MAX_NUM_AQOS_ENTRIES);
	if (ret)
		goto unwind;

	ret = dlb_bitmap_fill(hw->pf.avail_aqed_freelist_entries);
	if (ret)
		goto unwind;

	for (i = 0; i < DLB_MAX_NUM_VFS; i++) {
		ret = dlb_bitmap_alloc(&hw->vf[i].avail_hist_list_entries,
				       DLB_MAX_NUM_HIST_LIST_ENTRIES);
		if (ret)
			goto unwind;

		ret = dlb_bitmap_alloc(&hw->vf[i].avail_qed_freelist_entries,
				       DLB_MAX_NUM_LDB_CREDITS);
		if (ret)
			goto unwind;

		ret = dlb_bitmap_alloc(&hw->vf[i].avail_dqed_freelist_entries,
				       DLB_MAX_NUM_DIR_CREDITS);
		if (ret)
			goto unwind;

		ret = dlb_bitmap_alloc(&hw->vf[i].avail_aqed_freelist_entries,
				       DLB_MAX_NUM_AQOS_ENTRIES);
		if (ret)
			goto unwind;

		ret = dlb_bitmap_zero(hw->vf[i].avail_hist_list_entries);
		if (ret)
			goto unwind;

		ret = dlb_bitmap_zero(hw->vf[i].avail_qed_freelist_entries);
		if (ret)
			goto unwind;

		ret = dlb_bitmap_zero(hw->vf[i].avail_dqed_freelist_entries);
		if (ret)
			goto unwind;

		ret = dlb_bitmap_zero(hw->vf[i].avail_aqed_freelist_entries);
		if (ret)
			goto unwind;
	}

	/* Initialize the hardware resource IDs */
	for (i = 0; i < DLB_MAX_NUM_DOMAINS; i++) {
		hw->domains[i].id.phys_id = i;
		hw->domains[i].id.vf_owned = false;
	}

	for (i = 0; i < DLB_MAX_NUM_LDB_QUEUES; i++) {
		hw->rsrcs.ldb_queues[i].id.phys_id = i;
		hw->rsrcs.ldb_queues[i].id.vf_owned = false;
	}

	for (i = 0; i < DLB_MAX_NUM_LDB_PORTS; i++) {
		hw->rsrcs.ldb_ports[i].id.phys_id = i;
		hw->rsrcs.ldb_ports[i].id.vf_owned = false;
	}

	for (i = 0; i < DLB_MAX_NUM_DIR_PORTS; i++) {
		hw->rsrcs.dir_pq_pairs[i].id.phys_id = i;
		hw->rsrcs.dir_pq_pairs[i].id.vf_owned = false;
	}

	for (i = 0; i < DLB_MAX_NUM_LDB_CREDIT_POOLS; i++) {
		hw->rsrcs.ldb_credit_pools[i].id.phys_id = i;
		hw->rsrcs.ldb_credit_pools[i].id.vf_owned = false;
	}

	for (i = 0; i < DLB_MAX_NUM_DIR_CREDIT_POOLS; i++) {
		hw->rsrcs.dir_credit_pools[i].id.phys_id = i;
		hw->rsrcs.dir_credit_pools[i].id.vf_owned = false;
	}

	for (i = 0; i < DLB_MAX_NUM_SEQUENCE_NUMBER_GROUPS; i++) {
		hw->rsrcs.sn_groups[i].id = i;
		/* Default mode (0) is 32 sequence numbers per queue */
		hw->rsrcs.sn_groups[i].mode = 0;
		hw->rsrcs.sn_groups[i].sequence_numbers_per_queue = 32;
		hw->rsrcs.sn_groups[i].slot_use_bitmap = 0;
	}

	return 0;

unwind:
	dlb_resource_free(hw);

	return ret;
}

void dlb_disable_dp_vasr_feature(struct dlb_hw *hw)
{
	union dlb_dp_dir_csr_ctrl r0;

	r0.val = DLB_CSR_RD(hw, DLB_DP_DIR_CSR_CTRL);

	r0.field.cfg_vasr_dis = 1;

	DLB_CSR_WR(hw, DLB_DP_DIR_CSR_CTRL, r0.val);
}
