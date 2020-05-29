// SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
/* Copyright(c) 2016-2020 Intel Corporation */

#include "uapi/linux/dlb2_user.h"

#include "dlb2_hw_types.h"
#include "dlb2_mbox.h"
#include "dlb2_osdep.h"
#include "dlb2_osdep_bitmap.h"
#include "dlb2_osdep_types.h"
#include "dlb2_regs.h"
#include "dlb2_resource.h"

#define DLB2_DOM_LIST_HEAD(head, type) \
	DLB2_LIST_HEAD((head), type, domain_list)

#define DLB2_FUNC_LIST_HEAD(head, type) \
	DLB2_LIST_HEAD((head), type, func_list)

#define DLB2_DOM_LIST_FOR(head, ptr, iter) \
	DLB2_LIST_FOR_EACH(head, ptr, domain_list, iter)

#define DLB2_FUNC_LIST_FOR(head, ptr, iter) \
	DLB2_LIST_FOR_EACH(head, ptr, func_list, iter)

#define DLB2_DOM_LIST_FOR_SAFE(head, ptr, ptr_tmp, it, it_tmp) \
	DLB2_LIST_FOR_EACH_SAFE((head), ptr, ptr_tmp, domain_list, it, it_tmp)

#define DLB2_FUNC_LIST_FOR_SAFE(head, ptr, ptr_tmp, it, it_tmp) \
	DLB2_LIST_FOR_EACH_SAFE((head), ptr, ptr_tmp, func_list, it, it_tmp)

/* The PF driver cannot assume that a register write will affect subsequent HCW
 * writes. To ensure a write completes, the driver must read back a CSR. This
 * function only need be called for configuration that can occur after the
 * domain has started; prior to starting, applications can't send HCWs.
 */
static inline void dlb2_flush_csr(struct dlb2_hw *hw)
{
	DLB2_CSR_RD(hw, DLB2_SYS_TOTAL_VAS);
}

static void dlb2_init_fn_rsrc_lists(struct dlb2_function_resources *rsrc)
{
	int i;

	dlb2_list_init_head(&rsrc->avail_domains);
	dlb2_list_init_head(&rsrc->used_domains);
	dlb2_list_init_head(&rsrc->avail_ldb_queues);
	dlb2_list_init_head(&rsrc->avail_dir_pq_pairs);

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++)
		dlb2_list_init_head(&rsrc->avail_ldb_ports[i]);
}

static void dlb2_init_domain_rsrc_lists(struct dlb2_domain *domain)
{
	int i;

	dlb2_list_init_head(&domain->used_ldb_queues);
	dlb2_list_init_head(&domain->used_dir_pq_pairs);
	dlb2_list_init_head(&domain->avail_ldb_queues);
	dlb2_list_init_head(&domain->avail_dir_pq_pairs);

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++)
		dlb2_list_init_head(&domain->used_ldb_ports[i]);
	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++)
		dlb2_list_init_head(&domain->avail_ldb_ports[i]);
}

int dlb2_resource_init(struct dlb2_hw *hw)
{
	struct dlb2_list_entry *list;
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
	u32 init_ldb_port_allocation[DLB2_MAX_NUM_LDB_PORTS] = {
		0,  7,  14,  5, 12,  3, 10,  1,  8, 15,  6, 13,  4, 11,  2,  9,
		16, 23, 30, 21, 28, 19, 26, 17, 24, 31, 22, 29, 20, 27, 18, 25,
		32, 39, 46, 37, 44, 35, 42, 33, 40, 47, 38, 45, 36, 43, 34, 41,
		48, 55, 62, 53, 60, 51, 58, 49, 56, 63, 54, 61, 52, 59, 50, 57,
	};

	/* Zero-out resource tracking data structures */
	memset(&hw->rsrcs, 0, sizeof(hw->rsrcs));
	memset(&hw->pf, 0, sizeof(hw->pf));

	dlb2_init_fn_rsrc_lists(&hw->pf);

	for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++) {
		memset(&hw->vdev[i], 0, sizeof(hw->vdev[i]));
		dlb2_init_fn_rsrc_lists(&hw->vdev[i]);
	}

	for (i = 0; i < DLB2_MAX_NUM_DOMAINS; i++) {
		memset(&hw->domains[i], 0, sizeof(hw->domains[i]));
		dlb2_init_domain_rsrc_lists(&hw->domains[i]);
		hw->domains[i].parent_func = &hw->pf;
	}

	/* Give all resources to the PF driver */
	hw->pf.num_avail_domains = DLB2_MAX_NUM_DOMAINS;
	for (i = 0; i < hw->pf.num_avail_domains; i++) {
		list = &hw->domains[i].func_list;

		dlb2_list_add(&hw->pf.avail_domains, list);
	}

	hw->pf.num_avail_ldb_queues = DLB2_MAX_NUM_LDB_QUEUES;
	for (i = 0; i < hw->pf.num_avail_ldb_queues; i++) {
		list = &hw->rsrcs.ldb_queues[i].func_list;

		dlb2_list_add(&hw->pf.avail_ldb_queues, list);
	}

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++)
		hw->pf.num_avail_ldb_ports[i] =
			DLB2_MAX_NUM_LDB_PORTS / DLB2_NUM_COS_DOMAINS;

	for (i = 0; i < DLB2_MAX_NUM_LDB_PORTS; i++) {
		int cos_id = i >> DLB2_NUM_COS_DOMAINS;
		struct dlb2_ldb_port *port;

		port = &hw->rsrcs.ldb_ports[init_ldb_port_allocation[i]];

		dlb2_list_add(&hw->pf.avail_ldb_ports[cos_id],
			      &port->func_list);
	}

	hw->pf.num_avail_dir_pq_pairs = DLB2_MAX_NUM_DIR_PORTS;
	for (i = 0; i < hw->pf.num_avail_dir_pq_pairs; i++) {
		list = &hw->rsrcs.dir_pq_pairs[i].func_list;

		dlb2_list_add(&hw->pf.avail_dir_pq_pairs, list);
	}

	hw->pf.num_avail_qed_entries = DLB2_MAX_NUM_LDB_CREDITS;
	hw->pf.num_avail_dqed_entries = DLB2_MAX_NUM_DIR_CREDITS;
	hw->pf.num_avail_aqed_entries = DLB2_MAX_NUM_AQED_ENTRIES;

	ret = dlb2_bitmap_alloc(hw,
				&hw->pf.avail_hist_list_entries,
				DLB2_MAX_NUM_HIST_LIST_ENTRIES);
	if (ret)
		return ret;

	dlb2_bitmap_fill(hw->pf.avail_hist_list_entries);

	for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++) {
		ret = dlb2_bitmap_alloc(hw,
					&hw->vdev[i].avail_hist_list_entries,
					DLB2_MAX_NUM_HIST_LIST_ENTRIES);
		if (ret)
			return ret;

		dlb2_bitmap_zero(hw->vdev[i].avail_hist_list_entries);
	}

	/* Initialize the hardware resource IDs */
	for (i = 0; i < DLB2_MAX_NUM_DOMAINS; i++) {
		hw->domains[i].id.phys_id = i;
		hw->domains[i].id.vdev_owned = false;
	}

	for (i = 0; i < DLB2_MAX_NUM_LDB_QUEUES; i++) {
		hw->rsrcs.ldb_queues[i].id.phys_id = i;
		hw->rsrcs.ldb_queues[i].id.vdev_owned = false;
	}

	for (i = 0; i < DLB2_MAX_NUM_LDB_PORTS; i++) {
		hw->rsrcs.ldb_ports[i].id.phys_id = i;
		hw->rsrcs.ldb_ports[i].id.vdev_owned = false;
	}

	for (i = 0; i < DLB2_MAX_NUM_DIR_PORTS; i++) {
		hw->rsrcs.dir_pq_pairs[i].id.phys_id = i;
		hw->rsrcs.dir_pq_pairs[i].id.vdev_owned = false;
	}

	for (i = 0; i < DLB2_MAX_NUM_SEQUENCE_NUMBER_GROUPS; i++) {
		hw->rsrcs.sn_groups[i].id = i;
		/* Default mode (0) is 64 sequence numbers per queue */
		hw->rsrcs.sn_groups[i].mode = 0;
		hw->rsrcs.sn_groups[i].sequence_numbers_per_queue = 64;
		hw->rsrcs.sn_groups[i].slot_use_bitmap = 0;
	}

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++)
		hw->cos_reservation[i] = 100 / DLB2_NUM_COS_DOMAINS;

	return 0;
}

void dlb2_resource_free(struct dlb2_hw *hw)
{
	int i;

	dlb2_bitmap_free(hw->pf.avail_hist_list_entries);

	for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++)
		dlb2_bitmap_free(hw->vdev[i].avail_hist_list_entries);
}

static struct dlb2_domain *dlb2_get_domain_from_id(struct dlb2_hw *hw,
						   u32 id,
						   bool vdev_req,
						   unsigned int vdev_id)
{
	struct dlb2_list_entry *iteration __attribute__((unused));
	struct dlb2_function_resources *rsrcs;
	struct dlb2_domain *domain;

	if (id >= DLB2_MAX_NUM_DOMAINS)
		return NULL;

	if (!vdev_req)
		return &hw->domains[id];

	rsrcs = &hw->vdev[vdev_id];

	DLB2_FUNC_LIST_FOR(rsrcs->used_domains, domain, iteration)
		if (domain->id.virt_id == id)
			return domain;

	return NULL;
}

static struct dlb2_ldb_port *dlb2_get_ldb_port_from_id(struct dlb2_hw *hw,
						       u32 id,
						       bool vdev_req,
						       unsigned int vdev_id)
{
	struct dlb2_list_entry *iter1 __attribute__((unused));
	struct dlb2_list_entry *iter2 __attribute__((unused));
	struct dlb2_function_resources *rsrcs;
	struct dlb2_ldb_port *port;
	struct dlb2_domain *domain;
	int i;

	if (id >= DLB2_MAX_NUM_LDB_PORTS)
		return NULL;

	rsrcs = (vdev_req) ? &hw->vdev[vdev_id] : &hw->pf;

	if (!vdev_req)
		return &hw->rsrcs.ldb_ports[id];

	DLB2_FUNC_LIST_FOR(rsrcs->used_domains, domain, iter1) {
		for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
			DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i],
					  port,
					  iter2)
				if (port->id.virt_id == id)
					return port;
		}
	}

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		DLB2_FUNC_LIST_FOR(rsrcs->avail_ldb_ports[i], port, iter1)
			if (port->id.virt_id == id)
				return port;
	}

	return NULL;
}

static struct dlb2_ldb_port *
dlb2_get_domain_used_ldb_port(u32 id,
			      bool vdev_req,
			      struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_ldb_port *port;
	int i;

	if (id >= DLB2_MAX_NUM_LDB_PORTS)
		return NULL;

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i], port, iter)
			if ((!vdev_req && port->id.phys_id == id) ||
			    (vdev_req && port->id.virt_id == id))
				return port;

		DLB2_DOM_LIST_FOR(domain->avail_ldb_ports[i], port, iter)
			if ((!vdev_req && port->id.phys_id == id) ||
			    (vdev_req && port->id.virt_id == id))
				return port;
	}

	return NULL;
}

static struct dlb2_ldb_port *
dlb2_get_domain_ldb_port(u32 id,
			 bool vdev_req,
			 struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_ldb_port *port;
	int i;

	if (id >= DLB2_MAX_NUM_LDB_PORTS)
		return NULL;

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i], port, iter)
			if ((!vdev_req && port->id.phys_id == id) ||
			    (vdev_req && port->id.virt_id == id))
				return port;

		DLB2_DOM_LIST_FOR(domain->avail_ldb_ports[i], port, iter)
			if ((!vdev_req && port->id.phys_id == id) ||
			    (vdev_req && port->id.virt_id == id))
				return port;
	}

	return NULL;
}

static struct dlb2_dir_pq_pair *dlb2_get_dir_pq_from_id(struct dlb2_hw *hw,
							u32 id,
							bool vdev_req,
							unsigned int vdev_id)
{
	struct dlb2_list_entry *iter1 __attribute__((unused));
	struct dlb2_list_entry *iter2 __attribute__((unused));
	struct dlb2_function_resources *rsrcs;
	struct dlb2_dir_pq_pair *port;
	struct dlb2_domain *domain;

	if (id >= DLB2_MAX_NUM_DIR_PORTS)
		return NULL;

	rsrcs = (vdev_req) ? &hw->vdev[vdev_id] : &hw->pf;

	if (!vdev_req)
		return &hw->rsrcs.dir_pq_pairs[id];

	DLB2_FUNC_LIST_FOR(rsrcs->used_domains, domain, iter1) {
		DLB2_DOM_LIST_FOR(domain->used_dir_pq_pairs, port, iter2)
			if (port->id.virt_id == id)
				return port;
	}

	DLB2_FUNC_LIST_FOR(rsrcs->avail_dir_pq_pairs, port, iter1)
		if (port->id.virt_id == id)
			return port;

	return NULL;
}

static struct dlb2_dir_pq_pair *
dlb2_get_domain_used_dir_pq(u32 id,
			    bool vdev_req,
			    struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_dir_pq_pair *port;

	if (id >= DLB2_MAX_NUM_DIR_PORTS)
		return NULL;

	DLB2_DOM_LIST_FOR(domain->used_dir_pq_pairs, port, iter)
		if ((!vdev_req && port->id.phys_id == id) ||
		    (vdev_req && port->id.virt_id == id))
			return port;

	return NULL;
}

static struct dlb2_dir_pq_pair *
dlb2_get_domain_dir_pq(u32 id,
		       bool vdev_req,
		       struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_dir_pq_pair *port;

	if (id >= DLB2_MAX_NUM_DIR_PORTS)
		return NULL;

	DLB2_DOM_LIST_FOR(domain->used_dir_pq_pairs, port, iter)
		if ((!vdev_req && port->id.phys_id == id) ||
		    (vdev_req && port->id.virt_id == id))
			return port;

	DLB2_DOM_LIST_FOR(domain->avail_dir_pq_pairs, port, iter)
		if ((!vdev_req && port->id.phys_id == id) ||
		    (vdev_req && port->id.virt_id == id))
			return port;

	return NULL;
}

static struct dlb2_ldb_queue *
dlb2_get_ldb_queue_from_id(struct dlb2_hw *hw,
			   u32 id,
			   bool vdev_req,
			   unsigned int vdev_id)
{
	struct dlb2_list_entry *iter1 __attribute__((unused));
	struct dlb2_list_entry *iter2 __attribute__((unused));
	struct dlb2_function_resources *rsrcs;
	struct dlb2_ldb_queue *queue;
	struct dlb2_domain *domain;

	if (id >= DLB2_MAX_NUM_LDB_QUEUES)
		return NULL;

	rsrcs = (vdev_req) ? &hw->vdev[vdev_id] : &hw->pf;

	if (!vdev_req)
		return &hw->rsrcs.ldb_queues[id];

	DLB2_FUNC_LIST_FOR(rsrcs->used_domains, domain, iter1) {
		DLB2_DOM_LIST_FOR(domain->used_ldb_queues, queue, iter2)
			if (queue->id.virt_id == id)
				return queue;
	}

	DLB2_FUNC_LIST_FOR(rsrcs->avail_ldb_queues, queue, iter1)
		if (queue->id.virt_id == id)
			return queue;

	return NULL;
}

static struct dlb2_ldb_queue *
dlb2_get_domain_ldb_queue(u32 id,
			  bool vdev_req,
			  struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_ldb_queue *queue;

	if (id >= DLB2_MAX_NUM_LDB_QUEUES)
		return NULL;

	DLB2_DOM_LIST_FOR(domain->used_ldb_queues, queue, iter)
		if ((!vdev_req && queue->id.phys_id == id) ||
		    (vdev_req && queue->id.virt_id == id))
			return queue;

	return NULL;
}

#define DLB2_XFER_LL_RSRC(dst, src, num, type_t, name) ({		     \
	struct dlb2_list_entry *it1 __attribute__((unused));		     \
	struct dlb2_list_entry *it2 __attribute__((unused));		     \
	struct dlb2_function_resources *_src = src;			     \
	struct dlb2_function_resources *_dst = dst;			     \
	type_t *ptr, *tmp __attribute__((unused));			     \
	unsigned int i = 0;						     \
									     \
	DLB2_FUNC_LIST_FOR_SAFE(_src->avail_##name##s, ptr, tmp, it1, it2) { \
		if (i++ == (num))					     \
			break;						     \
									     \
		dlb2_list_del(&_src->avail_##name##s, &ptr->func_list);     \
		dlb2_list_add(&_dst->avail_##name##s,  &ptr->func_list);    \
		_src->num_avail_##name##s--;				     \
		_dst->num_avail_##name##s++;				     \
	}								     \
})

#define DLB2_XFER_LL_IDX_RSRC(dst, src, num, idx, type_t, name) ({	       \
	struct dlb2_list_entry *it1 __attribute__((unused));		       \
	struct dlb2_list_entry *it2 __attribute__((unused));		       \
	struct dlb2_function_resources *_src = src;			       \
	struct dlb2_function_resources *_dst = dst;			       \
	type_t *ptr, *tmp __attribute__((unused));			       \
	unsigned int i = 0;						       \
									       \
	DLB2_FUNC_LIST_FOR_SAFE(_src->avail_##name##s[idx],		       \
				 ptr, tmp, it1, it2) {			       \
		if (i++ == (num))					       \
			break;						       \
									       \
		dlb2_list_del(&_src->avail_##name##s[idx], &ptr->func_list);  \
		dlb2_list_add(&_dst->avail_##name##s[idx],  &ptr->func_list); \
		_src->num_avail_##name##s[idx]--;			       \
		_dst->num_avail_##name##s[idx]++;			       \
	}								       \
})

#define DLB2_VF_ID_CLEAR(head, type_t) ({ \
	struct dlb2_list_entry *iter __attribute__((unused)); \
	type_t *var;					       \
							       \
	DLB2_FUNC_LIST_FOR(head, var, iter)		       \
		var->id.vdev_owned = false;		       \
})

int dlb2_update_vdev_sched_domains(struct dlb2_hw *hw, u32 id, u32 num)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_function_resources *src, *dst;
	struct dlb2_domain *domain;
	unsigned int orig;
	int ret;

	if (id >= DLB2_MAX_NUM_VDEVS)
		return -EINVAL;

	src = &hw->pf;
	dst = &hw->vdev[id];

	/* If the VF is locked, its resource assignment can't be changed */
	if (dlb2_vdev_is_locked(hw, id))
		return -EPERM;

	orig = dst->num_avail_domains;

	/* Detach the destination VF's current resources before checking if
	 * enough are available, and set their IDs accordingly.
	 */
	DLB2_VF_ID_CLEAR(dst->avail_domains, struct dlb2_domain);

	DLB2_XFER_LL_RSRC(src, dst, orig, struct dlb2_domain, domain);

	/* Are there enough available resources to satisfy the request? */
	if (num > src->num_avail_domains) {
		num = orig;
		ret = -EINVAL;
	} else {
		ret = 0;
	}

	DLB2_XFER_LL_RSRC(dst, src, num, struct dlb2_domain, domain);

	/* Set the domains' VF backpointer */
	DLB2_FUNC_LIST_FOR(dst->avail_domains, domain, iter)
		domain->parent_func = dst;

	return ret;
}

int dlb2_update_vdev_ldb_queues(struct dlb2_hw *hw, u32 id, u32 num)
{
	struct dlb2_function_resources *src, *dst;
	unsigned int orig;
	int ret;

	if (id >= DLB2_MAX_NUM_VDEVS)
		return -EINVAL;

	src = &hw->pf;
	dst = &hw->vdev[id];

	/* If the VF is locked, its resource assignment can't be changed */
	if (dlb2_vdev_is_locked(hw, id))
		return -EPERM;

	orig = dst->num_avail_ldb_queues;

	/* Detach the destination VF's current resources before checking if
	 * enough are available, and set their IDs accordingly.
	 */
	DLB2_VF_ID_CLEAR(dst->avail_ldb_queues, struct dlb2_ldb_queue);

	DLB2_XFER_LL_RSRC(src, dst, orig, struct dlb2_ldb_queue, ldb_queue);

	/* Are there enough available resources to satisfy the request? */
	if (num > src->num_avail_ldb_queues) {
		num = orig;
		ret = -EINVAL;
	} else {
		ret = 0;
	}

	DLB2_XFER_LL_RSRC(dst, src, num, struct dlb2_ldb_queue, ldb_queue);

	return ret;
}

int dlb2_update_vdev_ldb_cos_ports(struct dlb2_hw *hw,
				   u32 id,
				   u32 cos,
				   u32 num)
{
	struct dlb2_function_resources *src, *dst;
	unsigned int orig;
	int ret;

	if (id >= DLB2_MAX_NUM_VDEVS)
		return -EINVAL;

	src = &hw->pf;
	dst = &hw->vdev[id];

	/* If the VF is locked, its resource assignment can't be changed */
	if (dlb2_vdev_is_locked(hw, id))
		return -EPERM;

	orig = dst->num_avail_ldb_ports[cos];

	/* Detach the destination VF's current resources before checking if
	 * enough are available, and set their IDs accordingly.
	 */
	DLB2_VF_ID_CLEAR(dst->avail_ldb_ports[cos], struct dlb2_ldb_port);

	DLB2_XFER_LL_IDX_RSRC(src, dst, orig, cos,
			      struct dlb2_ldb_port, ldb_port);

	/* Are there enough available resources to satisfy the request? */
	if (num > src->num_avail_ldb_ports[cos]) {
		num = orig;
		ret = -EINVAL;
	} else {
		ret = 0;
	}

	DLB2_XFER_LL_IDX_RSRC(dst, src, num, cos,
			      struct dlb2_ldb_port, ldb_port);

	return ret;
}

static int dlb2_add_vdev_ldb_ports(struct dlb2_hw *hw, u32 id, u32 num)
{
	struct dlb2_function_resources *src, *dst;
	u32 avail, orig[DLB2_NUM_COS_DOMAINS];
	int ret, i;

	if (num == 0)
		return 0;

	src = &hw->pf;
	dst = &hw->vdev[id];

	avail = 0;
	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++)
		avail += src->num_avail_ldb_ports[i];

	if (avail < num)
		return -EINVAL;

	/* Add ports to each CoS until num have been added */
	for (i = 0; i < DLB2_NUM_COS_DOMAINS && num > 0; i++) {
		u32 curr = dst->num_avail_ldb_ports[i];
		u32 num_to_add;

		avail = src->num_avail_ldb_ports[i];

		/* Don't attempt to add more than are available */
		num_to_add = num < avail ? num : avail;

		ret = dlb2_update_vdev_ldb_cos_ports(hw, id, i,
						     curr + num_to_add);
		if (ret)
			goto cleanup;

		orig[i] = curr;
		num -= num_to_add;
	}

	return 0;

cleanup:
	DLB2_BASE_ERR(hw,
		      "[%s()] Internal error: failed to add ldb ports\n",
		      __func__);

	/* Internal error, attempt to recover original configuration */
	for (i--; i >= 0; i--)
		dlb2_update_vdev_ldb_cos_ports(hw, id, i, orig[i]);

	return ret;
}

static int dlb2_del_vdev_ldb_ports(struct dlb2_hw *hw, u32 id, u32 num)
{
	struct dlb2_function_resources *dst;
	u32 orig[DLB2_NUM_COS_DOMAINS];
	int ret, i;

	if (num == 0)
		return 0;

	dst = &hw->vdev[id];

	/* Remove ports from each CoS until num have been removed */
	for (i = 0; i < DLB2_NUM_COS_DOMAINS && num > 0; i++) {
		u32 curr = dst->num_avail_ldb_ports[i];
		u32 num_to_del;

		/* Don't attempt to remove more than dst owns */
		num_to_del = num < curr ? num : curr;

		ret = dlb2_update_vdev_ldb_cos_ports(hw, id, i,
						     curr - num_to_del);
		if (ret)
			goto cleanup;

		orig[i] = curr;
		num -= curr;
	}

	return 0;

cleanup:
	DLB2_BASE_ERR(hw,
		      "[%s()] Internal error: failed to remove ldb ports\n",
		      __func__);

	/* Internal error, attempt to recover original configuration */
	for (i--; i >= 0; i--)
		dlb2_update_vdev_ldb_cos_ports(hw, id, i, orig[i]);

	return ret;
}

int dlb2_update_vdev_ldb_ports(struct dlb2_hw *hw, u32 id, u32 num)
{
	struct dlb2_function_resources *dst;
	unsigned int orig;
	int i;

	if (id >= DLB2_MAX_NUM_VDEVS)
		return -EINVAL;

	dst = &hw->vdev[id];

	/* If the VF is locked, its resource assignment can't be changed */
	if (dlb2_vdev_is_locked(hw, id))
		return -EPERM;

	orig = 0;

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++)
		orig += dst->num_avail_ldb_ports[i];

	if (orig == num)
		return 0;
	else if (orig < num)
		return dlb2_add_vdev_ldb_ports(hw, id, num - orig);
	else
		return dlb2_del_vdev_ldb_ports(hw, id, orig - num);
}

int dlb2_update_vdev_dir_ports(struct dlb2_hw *hw, u32 id, u32 num)
{
	struct dlb2_function_resources *src, *dst;
	unsigned int orig;
	int ret;

	if (id >= DLB2_MAX_NUM_VDEVS)
		return -EINVAL;

	src = &hw->pf;
	dst = &hw->vdev[id];

	/* If the VF is locked, its resource assignment can't be changed */
	if (dlb2_vdev_is_locked(hw, id))
		return -EPERM;

	orig = dst->num_avail_dir_pq_pairs;

	/* Detach the destination VF's current resources before checking if
	 * enough are available, and set their IDs accordingly.
	 */
	DLB2_VF_ID_CLEAR(dst->avail_dir_pq_pairs, struct dlb2_dir_pq_pair);

	DLB2_XFER_LL_RSRC(src, dst, orig,
			  struct dlb2_dir_pq_pair, dir_pq_pair);

	/* Are there enough available resources to satisfy the request? */
	if (num > src->num_avail_dir_pq_pairs) {
		num = orig;
		ret = -EINVAL;
	} else {
		ret = 0;
	}

	DLB2_XFER_LL_RSRC(dst, src, num,
			  struct dlb2_dir_pq_pair, dir_pq_pair);

	return ret;
}

static int dlb2_transfer_bitmap_resources(struct dlb2_bitmap *src,
					  struct dlb2_bitmap *dst,
					  u32 num)
{
	int orig, ret, base;

	/* Reassign the dest's bitmap entries to the source's before checking
	 * if a contiguous chunk of size 'num' is available. The reassignment
	 * may be necessary to create a sufficiently large contiguous chunk.
	 */
	orig = dlb2_bitmap_count(dst);

	dlb2_bitmap_or(src, src, dst);

	dlb2_bitmap_zero(dst);

	/* Are there enough available resources to satisfy the request? */
	base = dlb2_bitmap_find_set_bit_range(src, num);

	if (base == -ENOENT) {
		num = orig;
		base = dlb2_bitmap_find_set_bit_range(src, num);
		ret = -EINVAL;
	} else {
		ret = 0;
	}

	dlb2_bitmap_set_range(dst, base, num);

	dlb2_bitmap_clear_range(src, base, num);

	return ret;
}

int dlb2_update_vdev_ldb_credits(struct dlb2_hw *hw, u32 id, u32 num)
{
	struct dlb2_function_resources *src, *dst;
	u32 orig;

	if (id >= DLB2_MAX_NUM_VDEVS)
		return -EINVAL;

	src = &hw->pf;
	dst = &hw->vdev[id];

	/* If the VF is locked, its resource assignment can't be changed */
	if (dlb2_vdev_is_locked(hw, id))
		return -EPERM;

	/* Detach the destination VF's current resources before checking
	 * if enough are available.
	 */
	orig = dst->num_avail_qed_entries;
	src->num_avail_qed_entries += orig;
	dst->num_avail_qed_entries = 0;

	if (src->num_avail_qed_entries < num) {
		src->num_avail_qed_entries -= orig;
		dst->num_avail_qed_entries = orig;
		return -EINVAL;
	}

	src->num_avail_qed_entries -= num;
	dst->num_avail_qed_entries += num;

	return 0;
}

int dlb2_update_vdev_dir_credits(struct dlb2_hw *hw, u32 id, u32 num)
{
	struct dlb2_function_resources *src, *dst;
	u32 orig;

	if (id >= DLB2_MAX_NUM_VDEVS)
		return -EINVAL;

	src = &hw->pf;
	dst = &hw->vdev[id];

	/* If the VF is locked, its resource assignment can't be changed */
	if (dlb2_vdev_is_locked(hw, id))
		return -EPERM;

	/* Detach the destination VF's current resources before checking
	 * if enough are available.
	 */
	orig = dst->num_avail_dqed_entries;
	src->num_avail_dqed_entries += orig;
	dst->num_avail_dqed_entries = 0;

	if (src->num_avail_dqed_entries < num) {
		src->num_avail_dqed_entries -= orig;
		dst->num_avail_dqed_entries = orig;
		return -EINVAL;
	}

	src->num_avail_dqed_entries -= num;
	dst->num_avail_dqed_entries += num;

	return 0;
}

int dlb2_update_vdev_hist_list_entries(struct dlb2_hw *hw, u32 id, u32 num)
{
	struct dlb2_function_resources *src, *dst;

	if (id >= DLB2_MAX_NUM_VDEVS)
		return -EINVAL;

	src = &hw->pf;
	dst = &hw->vdev[id];

	/* If the VF is locked, its resource assignment can't be changed */
	if (dlb2_vdev_is_locked(hw, id))
		return -EPERM;

	return dlb2_transfer_bitmap_resources(src->avail_hist_list_entries,
					      dst->avail_hist_list_entries,
					      num);
}

int dlb2_update_vdev_atomic_inflights(struct dlb2_hw *hw, u32 id, u32 num)
{
	struct dlb2_function_resources *src, *dst;
	u32 orig;

	if (id >= DLB2_MAX_NUM_VDEVS)
		return -EINVAL;

	src = &hw->pf;
	dst = &hw->vdev[id];

	/* If the VF is locked, its resource assignment can't be changed */
	if (dlb2_vdev_is_locked(hw, id))
		return -EPERM;

	/* Detach the destination VF's current resources before checking
	 * if enough are available.
	 */
	orig = dst->num_avail_aqed_entries;
	src->num_avail_aqed_entries += orig;
	dst->num_avail_aqed_entries = 0;

	if (src->num_avail_aqed_entries < num) {
		src->num_avail_aqed_entries -= orig;
		dst->num_avail_aqed_entries = orig;
		return -EINVAL;
	}

	src->num_avail_aqed_entries -= num;
	dst->num_avail_aqed_entries += num;

	return 0;
}

static int dlb2_attach_ldb_queues(struct dlb2_hw *hw,
				  struct dlb2_function_resources *rsrcs,
				  struct dlb2_domain *domain,
				  u32 num_queues,
				  struct dlb2_cmd_response *resp)
{
	unsigned int i;

	if (rsrcs->num_avail_ldb_queues < num_queues) {
		resp->status = DLB2_ST_LDB_QUEUES_UNAVAILABLE;
		return -EINVAL;
	}

	for (i = 0; i < num_queues; i++) {
		struct dlb2_ldb_queue *queue;

		queue = DLB2_FUNC_LIST_HEAD(rsrcs->avail_ldb_queues,
					    typeof(*queue));
		if (!queue) {
			DLB2_BASE_ERR(hw,
				      "[%s()] Internal error: domain validation failed\n",
				      __func__);
			return -EFAULT;
		}

		dlb2_list_del(&rsrcs->avail_ldb_queues, &queue->func_list);

		queue->domain_id = domain->id;
		queue->owned = true;

		dlb2_list_add(&domain->avail_ldb_queues, &queue->domain_list);
	}

	rsrcs->num_avail_ldb_queues -= num_queues;

	return 0;
}

static struct dlb2_ldb_port *
dlb2_get_next_ldb_port(struct dlb2_hw *hw,
		       struct dlb2_function_resources *rsrcs,
		       u32 domain_id,
		       u32 cos_id)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_ldb_port *port;

	/* To reduce the odds of consecutive load-balanced ports mapping to the
	 * same queue(s), the driver attempts to allocate ports whose neighbors
	 * are owned by a different domain.
	 */
	DLB2_FUNC_LIST_FOR(rsrcs->avail_ldb_ports[cos_id], port, iter) {
		u32 next, prev;
		u32 phys_id;

		phys_id = port->id.phys_id;
		next = phys_id + 1;
		prev = phys_id - 1;

		if (phys_id == DLB2_MAX_NUM_LDB_PORTS - 1)
			next = 0;
		if (phys_id == 0)
			prev = DLB2_MAX_NUM_LDB_PORTS - 1;

		if (!hw->rsrcs.ldb_ports[next].owned ||
		    hw->rsrcs.ldb_ports[next].domain_id.phys_id == domain_id)
			continue;

		if (!hw->rsrcs.ldb_ports[prev].owned ||
		    hw->rsrcs.ldb_ports[prev].domain_id.phys_id == domain_id)
			continue;

		return port;
	}

	/* Failing that, the driver looks for a port with one neighbor owned by
	 * a different domain and the other unallocated.
	 */
	DLB2_FUNC_LIST_FOR(rsrcs->avail_ldb_ports[cos_id], port, iter) {
		u32 next, prev;
		u32 phys_id;

		phys_id = port->id.phys_id;
		next = phys_id + 1;
		prev = phys_id - 1;

		if (phys_id == DLB2_MAX_NUM_LDB_PORTS - 1)
			next = 0;
		if (phys_id == 0)
			prev = DLB2_MAX_NUM_LDB_PORTS - 1;

		if (!hw->rsrcs.ldb_ports[prev].owned &&
		    hw->rsrcs.ldb_ports[next].owned &&
		    hw->rsrcs.ldb_ports[next].domain_id.phys_id != domain_id)
			return port;

		if (!hw->rsrcs.ldb_ports[next].owned &&
		    hw->rsrcs.ldb_ports[prev].owned &&
		    hw->rsrcs.ldb_ports[prev].domain_id.phys_id != domain_id)
			return port;
	}

	/* Failing that, the driver looks for a port with both neighbors
	 * unallocated.
	 */
	DLB2_FUNC_LIST_FOR(rsrcs->avail_ldb_ports[cos_id], port, iter) {
		u32 next, prev;
		u32 phys_id;

		phys_id = port->id.phys_id;
		next = phys_id + 1;
		prev = phys_id - 1;

		if (phys_id == DLB2_MAX_NUM_LDB_PORTS - 1)
			next = 0;
		if (phys_id == 0)
			prev = DLB2_MAX_NUM_LDB_PORTS - 1;

		if (!hw->rsrcs.ldb_ports[prev].owned &&
		    !hw->rsrcs.ldb_ports[next].owned)
			return port;
	}

	/* If all else fails, the driver returns the next available port. */
	return DLB2_FUNC_LIST_HEAD(rsrcs->avail_ldb_ports[cos_id],
				   typeof(*port));
}

static int __dlb2_attach_ldb_ports(struct dlb2_hw *hw,
				   struct dlb2_function_resources *rsrcs,
				   struct dlb2_domain *domain,
				   u32 num_ports,
				   u32 cos_id,
				   struct dlb2_cmd_response *resp)
{
	unsigned int i;

	if (rsrcs->num_avail_ldb_ports[cos_id] < num_ports) {
		resp->status = DLB2_ST_LDB_PORTS_UNAVAILABLE;
		return -EINVAL;
	}

	for (i = 0; i < num_ports; i++) {
		struct dlb2_ldb_port *port;

		port = dlb2_get_next_ldb_port(hw, rsrcs,
					      domain->id.phys_id, cos_id);
		if (!port) {
			DLB2_BASE_ERR(hw,
				      "[%s()] Internal error: domain validation failed\n",
				      __func__);
			return -EFAULT;
		}

		dlb2_list_del(&rsrcs->avail_ldb_ports[cos_id],
			      &port->func_list);

		port->domain_id = domain->id;
		port->owned = true;

		dlb2_list_add(&domain->avail_ldb_ports[cos_id],
			      &port->domain_list);
	}

	rsrcs->num_avail_ldb_ports[cos_id] -= num_ports;

	return 0;
}

static int dlb2_attach_ldb_ports(struct dlb2_hw *hw,
				 struct dlb2_function_resources *rsrcs,
				 struct dlb2_domain *domain,
				 struct dlb2_create_sched_domain_args *args,
				 struct dlb2_cmd_response *resp)
{
	unsigned int i;
	int ret, j;

	if (args->cos_strict) {
		/* Allocate ports from specific classes-of-service */
		ret = __dlb2_attach_ldb_ports(hw,
					      rsrcs,
					      domain,
					      args->num_cos0_ldb_ports,
					      0,
					      resp);
		if (ret)
			return ret;

		ret = __dlb2_attach_ldb_ports(hw,
					      rsrcs,
					      domain,
					      args->num_cos1_ldb_ports,
					      1,
					      resp);
		if (ret)
			return ret;

		ret = __dlb2_attach_ldb_ports(hw,
					      rsrcs,
					      domain,
					      args->num_cos2_ldb_ports,
					      2,
					      resp);
		if (ret)
			return ret;

		ret = __dlb2_attach_ldb_ports(hw,
					      rsrcs,
					      domain,
					      args->num_cos3_ldb_ports,
					      3,
					      resp);
		if (ret)
			return ret;
	} else {
		u32 cos_id;

		/* Attempt to allocate from specific class-of-service, but
		 * fallback to the other classes if that fails.
		 */
		for (i = 0; i < args->num_cos0_ldb_ports; i++) {
			for (j = 0; j < DLB2_NUM_COS_DOMAINS; j++) {
				ret = __dlb2_attach_ldb_ports(hw,
							      rsrcs,
							      domain,
							      1,
							      j,
							      resp);
				if (ret == 0)
					break;
			}

			if (ret < 0)
				return ret;
		}

		for (i = 0; i < args->num_cos1_ldb_ports; i++) {
			for (j = 0; j < DLB2_NUM_COS_DOMAINS; j++) {
				cos_id = (1 + j) % DLB2_NUM_COS_DOMAINS;
				ret = __dlb2_attach_ldb_ports(hw,
							      rsrcs,
							      domain,
							      1,
							      cos_id,
							      resp);
				if (ret == 0)
					break;
			}

			if (ret < 0)
				return ret;
		}

		for (i = 0; i < args->num_cos2_ldb_ports; i++) {
			for (j = 0; j < DLB2_NUM_COS_DOMAINS; j++) {
				cos_id = (2 + j) % DLB2_NUM_COS_DOMAINS;
				ret = __dlb2_attach_ldb_ports(hw,
							      rsrcs,
							      domain,
							      1,
							      cos_id,
							      resp);
				if (ret == 0)
					break;
			}

			if (ret < 0)
				return ret;
		}

		for (i = 0; i < args->num_cos3_ldb_ports; i++) {
			for (j = 0; j < DLB2_NUM_COS_DOMAINS; j++) {
				cos_id = (3 + j) % DLB2_NUM_COS_DOMAINS;
				ret = __dlb2_attach_ldb_ports(hw,
							      rsrcs,
							      domain,
							      1,
							      cos_id,
							      resp);
				if (ret == 0)
					break;
			}

			if (ret < 0)
				return ret;
		}
	}

	/* Allocate num_ldb_ports from any class-of-service */
	for (i = 0; i < args->num_ldb_ports; i++) {
		for (j = 0; j < DLB2_NUM_COS_DOMAINS; j++) {
			ret = __dlb2_attach_ldb_ports(hw,
						      rsrcs,
						      domain,
						      1,
						      j,
						      resp);
			if (ret == 0)
				break;
		}

		if (ret < 0)
			return ret;
	}

	return 0;
}

static int dlb2_attach_dir_ports(struct dlb2_hw *hw,
				 struct dlb2_function_resources *rsrcs,
				 struct dlb2_domain *domain,
				 u32 num_ports,
				 struct dlb2_cmd_response *resp)
{
	unsigned int i;

	if (rsrcs->num_avail_dir_pq_pairs < num_ports) {
		resp->status = DLB2_ST_DIR_PORTS_UNAVAILABLE;
		return -EINVAL;
	}

	for (i = 0; i < num_ports; i++) {
		struct dlb2_dir_pq_pair *port;

		port = DLB2_FUNC_LIST_HEAD(rsrcs->avail_dir_pq_pairs,
					   typeof(*port));
		if (!port) {
			DLB2_BASE_ERR(hw,
				      "[%s()] Internal error: domain validation failed\n",
				      __func__);
			return -EFAULT;
		}

		dlb2_list_del(&rsrcs->avail_dir_pq_pairs, &port->func_list);

		port->domain_id = domain->id;
		port->owned = true;

		dlb2_list_add(&domain->avail_dir_pq_pairs, &port->domain_list);
	}

	rsrcs->num_avail_dir_pq_pairs -= num_ports;

	return 0;
}

static int dlb2_attach_ldb_credits(struct dlb2_function_resources *rsrcs,
				   struct dlb2_domain *domain,
				   u32 num_credits,
				   struct dlb2_cmd_response *resp)
{
	if (rsrcs->num_avail_qed_entries < num_credits) {
		resp->status = DLB2_ST_LDB_CREDITS_UNAVAILABLE;
		return -EINVAL;
	}

	rsrcs->num_avail_qed_entries -= num_credits;
	domain->num_ldb_credits += num_credits;
	return 0;
}

static int dlb2_attach_dir_credits(struct dlb2_function_resources *rsrcs,
				   struct dlb2_domain *domain,
				   u32 num_credits,
				   struct dlb2_cmd_response *resp)
{
	if (rsrcs->num_avail_dqed_entries < num_credits) {
		resp->status = DLB2_ST_DIR_CREDITS_UNAVAILABLE;
		return -EINVAL;
	}

	rsrcs->num_avail_dqed_entries -= num_credits;
	domain->num_dir_credits += num_credits;
	return 0;
}

static int dlb2_attach_atomic_inflights(struct dlb2_function_resources *rsrcs,
					struct dlb2_domain *domain,
					u32 num_atomic_inflights,
					struct dlb2_cmd_response *resp)
{
	if (rsrcs->num_avail_aqed_entries < num_atomic_inflights) {
		resp->status = DLB2_ST_ATOMIC_INFLIGHTS_UNAVAILABLE;
		return -EINVAL;
	}

	rsrcs->num_avail_aqed_entries -= num_atomic_inflights;
	domain->num_avail_aqed_entries += num_atomic_inflights;
	return 0;
}

static int
dlb2_attach_domain_hist_list_entries(struct dlb2_function_resources *rsrcs,
				     struct dlb2_domain *domain,
				     u32 num_hist_list_entries,
				     struct dlb2_cmd_response *resp)
{
	struct dlb2_bitmap *bitmap;
	int base;

	if (num_hist_list_entries) {
		bitmap = rsrcs->avail_hist_list_entries;

		base = dlb2_bitmap_find_set_bit_range(bitmap,
						      num_hist_list_entries);
		if (base < 0)
			goto error;

		domain->total_hist_list_entries = num_hist_list_entries;
		domain->avail_hist_list_entries = num_hist_list_entries;
		domain->hist_list_entry_base = base;
		domain->hist_list_entry_offset = 0;

		dlb2_bitmap_clear_range(bitmap, base, num_hist_list_entries);
	}
	return 0;

error:
	resp->status = DLB2_ST_HIST_LIST_ENTRIES_UNAVAILABLE;
	return -EINVAL;
}

static int
dlb2_verify_create_sched_dom_args(struct dlb2_function_resources *rsrcs,
				  struct dlb2_create_sched_domain_args *args,
				  struct dlb2_cmd_response *resp)
{
	u32 num_avail_ldb_ports, req_ldb_ports;
	struct dlb2_bitmap *avail_hl_entries;
	unsigned int max_contig_hl_range;
	int i;

	avail_hl_entries = rsrcs->avail_hist_list_entries;

	max_contig_hl_range = dlb2_bitmap_longest_set_range(avail_hl_entries);

	num_avail_ldb_ports = 0;
	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++)
		num_avail_ldb_ports += rsrcs->num_avail_ldb_ports[i];

	req_ldb_ports = args->num_cos0_ldb_ports + args->num_cos1_ldb_ports +
			args->num_cos2_ldb_ports + args->num_cos3_ldb_ports +
			args->num_ldb_ports;

	if (rsrcs->num_avail_domains < 1)
		resp->status = DLB2_ST_DOMAIN_UNAVAILABLE;
	else if (rsrcs->num_avail_ldb_queues < args->num_ldb_queues)
		resp->status = DLB2_ST_LDB_QUEUES_UNAVAILABLE;
	else if (req_ldb_ports > num_avail_ldb_ports)
		resp->status = DLB2_ST_LDB_PORTS_UNAVAILABLE;
	else if (args->cos_strict &&
		 args->num_cos0_ldb_ports > rsrcs->num_avail_ldb_ports[0])
		resp->status = DLB2_ST_LDB_PORTS_UNAVAILABLE;
	else if (args->cos_strict &&
		 args->num_cos1_ldb_ports > rsrcs->num_avail_ldb_ports[1])
		resp->status = DLB2_ST_LDB_PORTS_UNAVAILABLE;
	else if (args->cos_strict &&
		 args->num_cos2_ldb_ports > rsrcs->num_avail_ldb_ports[2])
		resp->status = DLB2_ST_LDB_PORTS_UNAVAILABLE;
	else if (args->cos_strict &&
		 args->num_cos3_ldb_ports > rsrcs->num_avail_ldb_ports[3])
		resp->status = DLB2_ST_LDB_PORTS_UNAVAILABLE;
	else if (args->num_ldb_queues > 0 && req_ldb_ports == 0)
		resp->status = DLB2_ST_LDB_PORT_REQUIRED_FOR_LDB_QUEUES;
	else if (rsrcs->num_avail_dir_pq_pairs < args->num_dir_ports)
		resp->status = DLB2_ST_DIR_PORTS_UNAVAILABLE;
	else if (rsrcs->num_avail_qed_entries < args->num_ldb_credits)
		resp->status = DLB2_ST_LDB_CREDITS_UNAVAILABLE;
	else if (rsrcs->num_avail_dqed_entries < args->num_dir_credits)
		resp->status = DLB2_ST_DIR_CREDITS_UNAVAILABLE;
	else if (rsrcs->num_avail_aqed_entries < args->num_atomic_inflights)
		resp->status = DLB2_ST_ATOMIC_INFLIGHTS_UNAVAILABLE;
	else if (max_contig_hl_range < args->num_hist_list_entries)
		resp->status = DLB2_ST_HIST_LIST_ENTRIES_UNAVAILABLE;

	if (resp->status)
		return -EINVAL;

	return 0;
}

static int
dlb2_verify_create_ldb_queue_args(struct dlb2_hw *hw,
				  u32 domain_id,
				  struct dlb2_create_ldb_queue_args *args,
				  struct dlb2_cmd_response *resp,
				  bool vdev_req,
				  unsigned int vdev_id)
{
	struct dlb2_domain *domain;
	int i;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);

	if (!domain) {
		resp->status = DLB2_ST_INVALID_DOMAIN_ID;
		return -EINVAL;
	}

	if (!domain->configured) {
		resp->status = DLB2_ST_DOMAIN_NOT_CONFIGURED;
		return -EINVAL;
	}

	if (domain->started) {
		resp->status = DLB2_ST_DOMAIN_STARTED;
		return -EINVAL;
	}

	if (dlb2_list_empty(&domain->avail_ldb_queues)) {
		resp->status = DLB2_ST_LDB_QUEUES_UNAVAILABLE;
		return -EINVAL;
	}

	if (args->num_sequence_numbers) {
		for (i = 0; i < DLB2_MAX_NUM_SEQUENCE_NUMBER_GROUPS; i++) {
			struct dlb2_sn_group *group = &hw->rsrcs.sn_groups[i];

			if (group->sequence_numbers_per_queue ==
			    args->num_sequence_numbers &&
			    !dlb2_sn_group_full(group))
				break;
		}

		if (i == DLB2_MAX_NUM_SEQUENCE_NUMBER_GROUPS) {
			resp->status = DLB2_ST_SEQUENCE_NUMBERS_UNAVAILABLE;
			return -EINVAL;
		}
	}

	if (args->num_qid_inflights > 4096) {
		resp->status = DLB2_ST_INVALID_QID_INFLIGHT_ALLOCATION;
		return -EINVAL;
	}

	/* Inflights must be <= number of sequence numbers if ordered */
	if (args->num_sequence_numbers != 0 &&
	    args->num_qid_inflights > args->num_sequence_numbers) {
		resp->status = DLB2_ST_INVALID_QID_INFLIGHT_ALLOCATION;
		return -EINVAL;
	}

	if (domain->num_avail_aqed_entries < args->num_atomic_inflights) {
		resp->status = DLB2_ST_ATOMIC_INFLIGHTS_UNAVAILABLE;
		return -EINVAL;
	}

	if (args->num_atomic_inflights &&
	    args->lock_id_comp_level != 0 &&
	    args->lock_id_comp_level != 64 &&
	    args->lock_id_comp_level != 128 &&
	    args->lock_id_comp_level != 256 &&
	    args->lock_id_comp_level != 512 &&
	    args->lock_id_comp_level != 1024 &&
	    args->lock_id_comp_level != 2048 &&
	    args->lock_id_comp_level != 4096 &&
	    args->lock_id_comp_level != 65536) {
		resp->status = DLB2_ST_INVALID_LOCK_ID_COMP_LEVEL;
		return -EINVAL;
	}

	return 0;
}

static int
dlb2_verify_create_dir_queue_args(struct dlb2_hw *hw,
				  u32 domain_id,
				  struct dlb2_create_dir_queue_args *args,
				  struct dlb2_cmd_response *resp,
				  bool vdev_req,
				  unsigned int vdev_id)
{
	struct dlb2_domain *domain;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);

	if (!domain) {
		resp->status = DLB2_ST_INVALID_DOMAIN_ID;
		return -EINVAL;
	}

	if (!domain->configured) {
		resp->status = DLB2_ST_DOMAIN_NOT_CONFIGURED;
		return -EINVAL;
	}

	if (domain->started) {
		resp->status = DLB2_ST_DOMAIN_STARTED;
		return -EINVAL;
	}

	/* If the user claims the port is already configured, validate the port
	 * ID, its domain, and whether the port is configured.
	 */
	if (args->port_id != -1) {
		struct dlb2_dir_pq_pair *port;

		port = dlb2_get_domain_used_dir_pq(args->port_id,
						   vdev_req,
						   domain);

		if (!port || port->domain_id.phys_id != domain->id.phys_id ||
		    !port->port_configured) {
			resp->status = DLB2_ST_INVALID_PORT_ID;
			return -EINVAL;
		}
	}

	/* If the queue's port is not configured, validate that a free
	 * port-queue pair is available.
	 */
	if (args->port_id == -1 &&
	    dlb2_list_empty(&domain->avail_dir_pq_pairs)) {
		resp->status = DLB2_ST_DIR_QUEUES_UNAVAILABLE;
		return -EINVAL;
	}

	return 0;
}

static void dlb2_configure_ldb_queue(struct dlb2_hw *hw,
				     struct dlb2_domain *domain,
				     struct dlb2_ldb_queue *queue,
				     struct dlb2_create_ldb_queue_args *args,
				     bool vdev_req,
				     unsigned int vdev_id)
{
	union dlb2_sys_vf_ldb_vqid_v r0 = { {0} };
	union dlb2_sys_vf_ldb_vqid2qid r1 = { {0} };
	union dlb2_sys_ldb_qid2vqid r2 = { {0} };
	union dlb2_sys_ldb_vasqid_v r3 = { {0} };
	union dlb2_lsp_qid_ldb_infl_lim r4 = { {0} };
	union dlb2_lsp_qid_aqed_active_lim r5 = { {0} };
	union dlb2_aqed_pipe_qid_hid_width r6 = { {0} };
	union dlb2_sys_ldb_qid_its r7 = { {0} };
	union dlb2_lsp_qid_atm_depth_thrsh r8 = { {0} };
	union dlb2_lsp_qid_naldb_depth_thrsh r9 = { {0} };
	union dlb2_aqed_pipe_qid_fid_lim r10 = { {0} };
	union dlb2_chp_ord_qid_sn_map r11 = { {0} };
	union dlb2_sys_ldb_qid_cfg_v r12 = { {0} };
	union dlb2_sys_ldb_qid_v r13 = { {0} };

	struct dlb2_sn_group *sn_group;
	unsigned int offs;

	/* QID write permissions are turned on when the domain is started */
	r3.field.vasqid_v = 0;

	offs = domain->id.phys_id * DLB2_MAX_NUM_LDB_QUEUES +
		queue->id.phys_id;

	DLB2_CSR_WR(hw, DLB2_SYS_LDB_VASQID_V(offs), r3.val);

	/* Unordered QIDs get 4K inflights, ordered get as many as the number
	 * of sequence numbers.
	 */
	r4.field.limit = args->num_qid_inflights;

	DLB2_CSR_WR(hw, DLB2_LSP_QID_LDB_INFL_LIM(queue->id.phys_id), r4.val);

	r5.field.limit = queue->aqed_limit;

	if (r5.field.limit > DLB2_MAX_NUM_AQED_ENTRIES)
		r5.field.limit = DLB2_MAX_NUM_AQED_ENTRIES;

	DLB2_CSR_WR(hw,
		    DLB2_LSP_QID_AQED_ACTIVE_LIM(queue->id.phys_id),
		    r5.val);

	switch (args->lock_id_comp_level) {
	case 64:
		r6.field.compress_code = 1;
		break;
	case 128:
		r6.field.compress_code = 2;
		break;
	case 256:
		r6.field.compress_code = 3;
		break;
	case 512:
		r6.field.compress_code = 4;
		break;
	case 1024:
		r6.field.compress_code = 5;
		break;
	case 2048:
		r6.field.compress_code = 6;
		break;
	case 4096:
		r6.field.compress_code = 7;
		break;
	case 0:
	case 65536:
		r6.field.compress_code = 0;
	}

	DLB2_CSR_WR(hw,
		    DLB2_AQED_PIPE_QID_HID_WIDTH(queue->id.phys_id),
		    r6.val);

	/* Don't timestamp QEs that pass through this queue */
	r7.field.qid_its = 0;

	DLB2_CSR_WR(hw,
		    DLB2_SYS_LDB_QID_ITS(queue->id.phys_id),
		    r7.val);

	r8.field.thresh = args->depth_threshold;

	DLB2_CSR_WR(hw,
		    DLB2_LSP_QID_ATM_DEPTH_THRSH(queue->id.phys_id),
		    r8.val);

	r9.field.thresh = args->depth_threshold;

	DLB2_CSR_WR(hw,
		    DLB2_LSP_QID_NALDB_DEPTH_THRSH(queue->id.phys_id),
		    r9.val);

	/* This register limits the number of inflight flows a queue can have
	 * at one time.  It has an upper bound of 2048, but can be
	 * over-subscribed. 512 is chosen so that a single queue doesn't use
	 * the entire atomic storage, but can use a substantial portion if
	 * needed.
	 */
	r10.field.qid_fid_limit = 512;

	DLB2_CSR_WR(hw,
		    DLB2_AQED_PIPE_QID_FID_LIM(queue->id.phys_id),
		    r10.val);

	/* Configure SNs */
	sn_group = &hw->rsrcs.sn_groups[queue->sn_group];
	r11.field.mode = sn_group->mode;
	r11.field.slot = queue->sn_slot;
	r11.field.grp  = sn_group->id;

	DLB2_CSR_WR(hw, DLB2_CHP_ORD_QID_SN_MAP(queue->id.phys_id), r11.val);

	r12.field.sn_cfg_v = (args->num_sequence_numbers != 0);
	r12.field.fid_cfg_v = (args->num_atomic_inflights != 0);

	DLB2_CSR_WR(hw, DLB2_SYS_LDB_QID_CFG_V(queue->id.phys_id), r12.val);

	if (vdev_req) {
		offs = vdev_id * DLB2_MAX_NUM_LDB_QUEUES + queue->id.virt_id;

		r0.field.vqid_v = 1;

		DLB2_CSR_WR(hw, DLB2_SYS_VF_LDB_VQID_V(offs), r0.val);

		r1.field.qid = queue->id.phys_id;

		DLB2_CSR_WR(hw, DLB2_SYS_VF_LDB_VQID2QID(offs), r1.val);

		r2.field.vqid = queue->id.virt_id;

		DLB2_CSR_WR(hw,
			    DLB2_SYS_LDB_QID2VQID(queue->id.phys_id),
			    r2.val);
	}

	r13.field.qid_v = 1;

	DLB2_CSR_WR(hw, DLB2_SYS_LDB_QID_V(queue->id.phys_id), r13.val);
}

static void dlb2_configure_dir_queue(struct dlb2_hw *hw,
				     struct dlb2_domain *domain,
				     struct dlb2_dir_pq_pair *queue,
				     struct dlb2_create_dir_queue_args *args,
				     bool vdev_req,
				     unsigned int vdev_id)
{
	union dlb2_sys_dir_vasqid_v r0 = { {0} };
	union dlb2_sys_dir_qid_its r1 = { {0} };
	union dlb2_lsp_qid_dir_depth_thrsh r2 = { {0} };
	union dlb2_sys_dir_qid_v r5 = { {0} };

	unsigned int offs;

	/* QID write permissions are turned on when the domain is started */
	r0.field.vasqid_v = 0;

	offs = domain->id.phys_id * DLB2_MAX_NUM_DIR_QUEUES +
		queue->id.phys_id;

	DLB2_CSR_WR(hw, DLB2_SYS_DIR_VASQID_V(offs), r0.val);

	/* Don't timestamp QEs that pass through this queue */
	r1.field.qid_its = 0;

	DLB2_CSR_WR(hw,
		    DLB2_SYS_DIR_QID_ITS(queue->id.phys_id),
		    r1.val);

	r2.field.thresh = args->depth_threshold;

	DLB2_CSR_WR(hw,
		    DLB2_LSP_QID_DIR_DEPTH_THRSH(queue->id.phys_id),
		    r2.val);

	if (vdev_req) {
		union dlb2_sys_vf_dir_vqid_v r3 = { {0} };
		union dlb2_sys_vf_dir_vqid2qid r4 = { {0} };

		offs = vdev_id * DLB2_MAX_NUM_DIR_QUEUES + queue->id.virt_id;

		r3.field.vqid_v = 1;

		DLB2_CSR_WR(hw, DLB2_SYS_VF_DIR_VQID_V(offs), r3.val);

		r4.field.qid = queue->id.phys_id;

		DLB2_CSR_WR(hw, DLB2_SYS_VF_DIR_VQID2QID(offs), r4.val);
	}

	r5.field.qid_v = 1;

	DLB2_CSR_WR(hw, DLB2_SYS_DIR_QID_V(queue->id.phys_id), r5.val);

	queue->queue_configured = true;
}

static int
dlb2_verify_create_ldb_port_args(struct dlb2_hw *hw,
				 u32 domain_id,
				 uintptr_t cq_dma_base,
				 struct dlb2_create_ldb_port_args *args,
				 struct dlb2_cmd_response *resp,
				 bool vdev_req,
				 unsigned int vdev_id)
{
	struct dlb2_domain *domain;
	int i;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);

	if (!domain) {
		resp->status = DLB2_ST_INVALID_DOMAIN_ID;
		return -EINVAL;
	}

	if (!domain->configured) {
		resp->status = DLB2_ST_DOMAIN_NOT_CONFIGURED;
		return -EINVAL;
	}

	if (domain->started) {
		resp->status = DLB2_ST_DOMAIN_STARTED;
		return -EINVAL;
	}

	if (args->cos_id >= DLB2_NUM_COS_DOMAINS) {
		resp->status = DLB2_ST_INVALID_COS_ID;
		return -EINVAL;
	}

	if (args->cos_strict) {
		if (dlb2_list_empty(&domain->avail_ldb_ports[args->cos_id])) {
			resp->status = DLB2_ST_LDB_PORTS_UNAVAILABLE;
			return -EINVAL;
		}
	} else {
		for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
			if (!dlb2_list_empty(&domain->avail_ldb_ports[i]))
				break;
		}

		if (i == DLB2_NUM_COS_DOMAINS) {
			resp->status = DLB2_ST_LDB_PORTS_UNAVAILABLE;
			return -EINVAL;
		}
	}

	/* Check cache-line alignment */
	if ((cq_dma_base & 0x3F) != 0) {
		resp->status = DLB2_ST_INVALID_CQ_VIRT_ADDR;
		return -EINVAL;
	}

	if (args->cq_depth != 1 &&
	    args->cq_depth != 2 &&
	    args->cq_depth != 4 &&
	    args->cq_depth != 8 &&
	    args->cq_depth != 16 &&
	    args->cq_depth != 32 &&
	    args->cq_depth != 64 &&
	    args->cq_depth != 128 &&
	    args->cq_depth != 256 &&
	    args->cq_depth != 512 &&
	    args->cq_depth != 1024) {
		resp->status = DLB2_ST_INVALID_CQ_DEPTH;
		return -EINVAL;
	}

	/* The history list size must be >= 1 */
	if (!args->cq_history_list_size) {
		resp->status = DLB2_ST_INVALID_HIST_LIST_DEPTH;
		return -EINVAL;
	}

	if (args->cq_history_list_size > domain->avail_hist_list_entries) {
		resp->status = DLB2_ST_HIST_LIST_ENTRIES_UNAVAILABLE;
		return -EINVAL;
	}

	return 0;
}

static int
dlb2_verify_create_dir_port_args(struct dlb2_hw *hw,
				 u32 domain_id,
				 uintptr_t cq_dma_base,
				 struct dlb2_create_dir_port_args *args,
				 struct dlb2_cmd_response *resp,
				 bool vdev_req,
				 unsigned int vdev_id)
{
	struct dlb2_domain *domain;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);

	if (!domain) {
		resp->status = DLB2_ST_INVALID_DOMAIN_ID;
		return -EINVAL;
	}

	if (!domain->configured) {
		resp->status = DLB2_ST_DOMAIN_NOT_CONFIGURED;
		return -EINVAL;
	}

	if (domain->started) {
		resp->status = DLB2_ST_DOMAIN_STARTED;
		return -EINVAL;
	}

	/* If the user claims the queue is already configured, validate
	 * the queue ID, its domain, and whether the queue is configured.
	 */
	if (args->queue_id != -1) {
		struct dlb2_dir_pq_pair *queue;

		queue = dlb2_get_domain_used_dir_pq(args->queue_id,
						    vdev_req,
						    domain);

		if (!queue || queue->domain_id.phys_id != domain->id.phys_id ||
		    !queue->queue_configured) {
			resp->status = DLB2_ST_INVALID_DIR_QUEUE_ID;
			return -EINVAL;
		}
	}

	/* If the port's queue is not configured, validate that a free
	 * port-queue pair is available.
	 */
	if (args->queue_id == -1 &&
	    dlb2_list_empty(&domain->avail_dir_pq_pairs)) {
		resp->status = DLB2_ST_DIR_PORTS_UNAVAILABLE;
		return -EINVAL;
	}

	/* Check cache-line alignment */
	if ((cq_dma_base & 0x3F) != 0) {
		resp->status = DLB2_ST_INVALID_CQ_VIRT_ADDR;
		return -EINVAL;
	}

	if (args->cq_depth != 1 &&
	    args->cq_depth != 2 &&
	    args->cq_depth != 4 &&
	    args->cq_depth != 8 &&
	    args->cq_depth != 16 &&
	    args->cq_depth != 32 &&
	    args->cq_depth != 64 &&
	    args->cq_depth != 128 &&
	    args->cq_depth != 256 &&
	    args->cq_depth != 512 &&
	    args->cq_depth != 1024) {
		resp->status = DLB2_ST_INVALID_CQ_DEPTH;
		return -EINVAL;
	}

	return 0;
}

static int dlb2_verify_start_domain_args(struct dlb2_hw *hw,
					 u32 domain_id,
					 struct dlb2_cmd_response *resp,
					 bool vdev_req,
					 unsigned int vdev_id)
{
	struct dlb2_domain *domain;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);

	if (!domain) {
		resp->status = DLB2_ST_INVALID_DOMAIN_ID;
		return -EINVAL;
	}

	if (!domain->configured) {
		resp->status = DLB2_ST_DOMAIN_NOT_CONFIGURED;
		return -EINVAL;
	}

	if (domain->started) {
		resp->status = DLB2_ST_DOMAIN_STARTED;
		return -EINVAL;
	}

	return 0;
}

static int dlb2_verify_map_qid_args(struct dlb2_hw *hw,
				    u32 domain_id,
				    struct dlb2_map_qid_args *args,
				    struct dlb2_cmd_response *resp,
				    bool vdev_req,
				    unsigned int vdev_id)
{
	struct dlb2_domain *domain;
	struct dlb2_ldb_port *port;
	struct dlb2_ldb_queue *queue;
	int id;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);

	if (!domain) {
		resp->status = DLB2_ST_INVALID_DOMAIN_ID;
		return -EINVAL;
	}

	if (!domain->configured) {
		resp->status = DLB2_ST_DOMAIN_NOT_CONFIGURED;
		return -EINVAL;
	}

	id = args->port_id;

	port = dlb2_get_domain_used_ldb_port(id, vdev_req, domain);

	if (!port || !port->configured) {
		resp->status = DLB2_ST_INVALID_PORT_ID;
		return -EINVAL;
	}

	if (args->priority >= DLB2_QID_PRIORITIES) {
		resp->status = DLB2_ST_INVALID_PRIORITY;
		return -EINVAL;
	}

	queue = dlb2_get_domain_ldb_queue(args->qid, vdev_req, domain);

	if (!queue || !queue->configured) {
		resp->status = DLB2_ST_INVALID_QID;
		return -EINVAL;
	}

	if (queue->domain_id.phys_id != domain->id.phys_id) {
		resp->status = DLB2_ST_INVALID_QID;
		return -EINVAL;
	}

	if (port->domain_id.phys_id != domain->id.phys_id) {
		resp->status = DLB2_ST_INVALID_PORT_ID;
		return -EINVAL;
	}

	return 0;
}

static bool dlb2_port_find_slot(struct dlb2_ldb_port *port,
				enum dlb2_qid_map_state state,
				int *slot)
{
	int i;

	for (i = 0; i < DLB2_MAX_NUM_QIDS_PER_LDB_CQ; i++) {
		if (port->qid_map[i].state == state)
			break;
	}

	*slot = i;

	return (i < DLB2_MAX_NUM_QIDS_PER_LDB_CQ);
}

static bool dlb2_port_find_slot_queue(struct dlb2_ldb_port *port,
				      enum dlb2_qid_map_state state,
				      struct dlb2_ldb_queue *queue,
				      int *slot)
{
	int i;

	for (i = 0; i < DLB2_MAX_NUM_QIDS_PER_LDB_CQ; i++) {
		if (port->qid_map[i].state == state &&
		    port->qid_map[i].qid == queue->id.phys_id)
			break;
	}

	*slot = i;

	return (i < DLB2_MAX_NUM_QIDS_PER_LDB_CQ);
}

static bool
dlb2_port_find_slot_with_pending_map_queue(struct dlb2_ldb_port *port,
					   struct dlb2_ldb_queue *queue,
					   int *slot)
{
	int i;

	for (i = 0; i < DLB2_MAX_NUM_QIDS_PER_LDB_CQ; i++) {
		struct dlb2_ldb_port_qid_map *map = &port->qid_map[i];

		if (map->state == DLB2_QUEUE_UNMAP_IN_PROG_PENDING_MAP &&
		    map->pending_qid == queue->id.phys_id)
			break;
	}

	*slot = i;

	return (i < DLB2_MAX_NUM_QIDS_PER_LDB_CQ);
}

static int dlb2_port_slot_state_transition(struct dlb2_hw *hw,
					   struct dlb2_ldb_port *port,
					   struct dlb2_ldb_queue *queue,
					   int slot,
					   enum dlb2_qid_map_state new_state)
{
	enum dlb2_qid_map_state curr_state = port->qid_map[slot].state;
	struct dlb2_domain *domain;
	int domain_id;

	domain_id = port->domain_id.phys_id;

	domain = dlb2_get_domain_from_id(hw, domain_id, false, 0);
	if (!domain) {
		DLB2_BASE_ERR(hw,
			      "[%s()] Internal error: unable to find domain %d\n",
			      __func__, domain_id);
		return -EINVAL;
	}

	switch (curr_state) {
	case DLB2_QUEUE_UNMAPPED:
		switch (new_state) {
		case DLB2_QUEUE_MAPPED:
			queue->num_mappings++;
			port->num_mappings++;
			break;
		case DLB2_QUEUE_MAP_IN_PROG:
			queue->num_pending_additions++;
			domain->num_pending_additions++;
			break;
		default:
			goto error;
		}
		break;
	case DLB2_QUEUE_MAPPED:
		switch (new_state) {
		case DLB2_QUEUE_UNMAPPED:
			queue->num_mappings--;
			port->num_mappings--;
			break;
		case DLB2_QUEUE_UNMAP_IN_PROG:
			port->num_pending_removals++;
			domain->num_pending_removals++;
			break;
		case DLB2_QUEUE_MAPPED:
			/* Priority change, nothing to update */
			break;
		default:
			goto error;
		}
		break;
	case DLB2_QUEUE_MAP_IN_PROG:
		switch (new_state) {
		case DLB2_QUEUE_UNMAPPED:
			queue->num_pending_additions--;
			domain->num_pending_additions--;
			break;
		case DLB2_QUEUE_MAPPED:
			queue->num_mappings++;
			port->num_mappings++;
			queue->num_pending_additions--;
			domain->num_pending_additions--;
			break;
		default:
			goto error;
		}
		break;
	case DLB2_QUEUE_UNMAP_IN_PROG:
		switch (new_state) {
		case DLB2_QUEUE_UNMAPPED:
			port->num_pending_removals--;
			domain->num_pending_removals--;
			queue->num_mappings--;
			port->num_mappings--;
			break;
		case DLB2_QUEUE_MAPPED:
			port->num_pending_removals--;
			domain->num_pending_removals--;
			break;
		case DLB2_QUEUE_UNMAP_IN_PROG_PENDING_MAP:
			/* Nothing to update */
			break;
		default:
			goto error;
		}
		break;
	case DLB2_QUEUE_UNMAP_IN_PROG_PENDING_MAP:
		switch (new_state) {
		case DLB2_QUEUE_UNMAP_IN_PROG:
			/* Nothing to update */
			break;
		case DLB2_QUEUE_UNMAPPED:
			/* An UNMAP_IN_PROG_PENDING_MAP slot briefly
			 * becomes UNMAPPED before it transitions to
			 * MAP_IN_PROG.
			 */
			queue->num_mappings--;
			port->num_mappings--;
			port->num_pending_removals--;
			domain->num_pending_removals--;
			break;
		default:
			goto error;
		}
		break;
	default:
		goto error;
	}

	port->qid_map[slot].state = new_state;

	DLB2_BASE_DBG(hw,
		      "[%s()] queue %d -> port %d state transition (%d -> %d)\n",
		      __func__, queue->id.phys_id, port->id.phys_id,
		      curr_state, new_state);
	return 0;

error:
	DLB2_BASE_ERR(hw,
		      "[%s()] Internal error: invalid queue %d -> port %d state transition (%d -> %d)\n",
		      __func__, queue->id.phys_id, port->id.phys_id,
		      curr_state, new_state);
	return -EFAULT;
}

static int dlb2_verify_map_qid_slot_available(struct dlb2_ldb_port *port,
					      struct dlb2_ldb_queue *queue,
					      struct dlb2_cmd_response *resp)
{
	enum dlb2_qid_map_state state;
	int i;

	/* Unused slot available? */
	if (port->num_mappings < DLB2_MAX_NUM_QIDS_PER_LDB_CQ)
		return 0;

	/* If the queue is already mapped (from the application's perspective),
	 * this is simply a priority update.
	 */
	state = DLB2_QUEUE_MAPPED;
	if (dlb2_port_find_slot_queue(port, state, queue, &i))
		return 0;

	state = DLB2_QUEUE_MAP_IN_PROG;
	if (dlb2_port_find_slot_queue(port, state, queue, &i))
		return 0;

	if (dlb2_port_find_slot_with_pending_map_queue(port, queue, &i))
		return 0;

	/* If the slot contains an unmap in progress, it's considered
	 * available.
	 */
	state = DLB2_QUEUE_UNMAP_IN_PROG;
	if (dlb2_port_find_slot(port, state, &i))
		return 0;

	state = DLB2_QUEUE_UNMAPPED;
	if (dlb2_port_find_slot(port, state, &i))
		return 0;

	resp->status = DLB2_ST_NO_QID_SLOTS_AVAILABLE;
	return -EINVAL;
}

static int dlb2_verify_unmap_qid_args(struct dlb2_hw *hw,
				      u32 domain_id,
				      struct dlb2_unmap_qid_args *args,
				      struct dlb2_cmd_response *resp,
				      bool vdev_req,
				      unsigned int vdev_id)
{
	enum dlb2_qid_map_state state;
	struct dlb2_domain *domain;
	struct dlb2_ldb_port *port;
	struct dlb2_ldb_queue *queue;
	int slot;
	int id;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);

	if (!domain) {
		resp->status = DLB2_ST_INVALID_DOMAIN_ID;
		return -EINVAL;
	}

	if (!domain->configured) {
		resp->status = DLB2_ST_DOMAIN_NOT_CONFIGURED;
		return -EINVAL;
	}

	id = args->port_id;

	port = dlb2_get_domain_used_ldb_port(id, vdev_req, domain);

	if (!port || !port->configured) {
		resp->status = DLB2_ST_INVALID_PORT_ID;
		return -EINVAL;
	}

	if (port->domain_id.phys_id != domain->id.phys_id) {
		resp->status = DLB2_ST_INVALID_PORT_ID;
		return -EINVAL;
	}

	queue = dlb2_get_domain_ldb_queue(args->qid, vdev_req, domain);

	if (!queue || !queue->configured) {
		DLB2_BASE_ERR(hw, "[%s()] Can't unmap unconfigured queue %d\n",
			      __func__, args->qid);
		resp->status = DLB2_ST_INVALID_QID;
		return -EINVAL;
	}

	/* Verify that the port has the queue mapped. From the application's
	 * perspective a queue is mapped if it is actually mapped, the map is
	 * in progress, or the map is blocked pending an unmap.
	 */
	state = DLB2_QUEUE_MAPPED;
	if (dlb2_port_find_slot_queue(port, state, queue, &slot))
		return 0;

	state = DLB2_QUEUE_MAP_IN_PROG;
	if (dlb2_port_find_slot_queue(port, state, queue, &slot))
		return 0;

	if (dlb2_port_find_slot_with_pending_map_queue(port, queue, &slot))
		return 0;

	resp->status = DLB2_ST_INVALID_QID;
	return -EINVAL;
}

static int
dlb2_verify_enable_ldb_port_args(struct dlb2_hw *hw,
				 u32 domain_id,
				 struct dlb2_enable_ldb_port_args *args,
				 struct dlb2_cmd_response *resp,
				 bool vdev_req,
				 unsigned int vdev_id)
{
	struct dlb2_domain *domain;
	struct dlb2_ldb_port *port;
	int id;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);

	if (!domain) {
		resp->status = DLB2_ST_INVALID_DOMAIN_ID;
		return -EINVAL;
	}

	if (!domain->configured) {
		resp->status = DLB2_ST_DOMAIN_NOT_CONFIGURED;
		return -EINVAL;
	}

	id = args->port_id;

	port = dlb2_get_domain_used_ldb_port(id, vdev_req, domain);

	if (!port || !port->configured) {
		resp->status = DLB2_ST_INVALID_PORT_ID;
		return -EINVAL;
	}

	return 0;
}

static int
dlb2_verify_enable_dir_port_args(struct dlb2_hw *hw,
				 u32 domain_id,
				 struct dlb2_enable_dir_port_args *args,
				 struct dlb2_cmd_response *resp,
				 bool vdev_req,
				 unsigned int vdev_id)
{
	struct dlb2_domain *domain;
	struct dlb2_dir_pq_pair *port;
	int id;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);

	if (!domain) {
		resp->status = DLB2_ST_INVALID_DOMAIN_ID;
		return -EINVAL;
	}

	if (!domain->configured) {
		resp->status = DLB2_ST_DOMAIN_NOT_CONFIGURED;
		return -EINVAL;
	}

	id = args->port_id;

	port = dlb2_get_domain_used_dir_pq(id, vdev_req, domain);

	if (!port || !port->port_configured) {
		resp->status = DLB2_ST_INVALID_PORT_ID;
		return -EINVAL;
	}

	return 0;
}

static int
dlb2_verify_disable_ldb_port_args(struct dlb2_hw *hw,
				  u32 domain_id,
				  struct dlb2_disable_ldb_port_args *args,
				  struct dlb2_cmd_response *resp,
				  bool vdev_req,
				  unsigned int vdev_id)
{
	struct dlb2_domain *domain;
	struct dlb2_ldb_port *port;
	int id;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);

	if (!domain) {
		resp->status = DLB2_ST_INVALID_DOMAIN_ID;
		return -EINVAL;
	}

	if (!domain->configured) {
		resp->status = DLB2_ST_DOMAIN_NOT_CONFIGURED;
		return -EINVAL;
	}

	id = args->port_id;

	port = dlb2_get_domain_used_ldb_port(id, vdev_req, domain);

	if (!port || !port->configured) {
		resp->status = DLB2_ST_INVALID_PORT_ID;
		return -EINVAL;
	}

	return 0;
}

static int
dlb2_verify_disable_dir_port_args(struct dlb2_hw *hw,
				  u32 domain_id,
				  struct dlb2_disable_dir_port_args *args,
				  struct dlb2_cmd_response *resp,
				  bool vdev_req,
				  unsigned int vdev_id)
{
	struct dlb2_domain *domain;
	struct dlb2_dir_pq_pair *port;
	int id;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);

	if (!domain) {
		resp->status = DLB2_ST_INVALID_DOMAIN_ID;
		return -EINVAL;
	}

	if (!domain->configured) {
		resp->status = DLB2_ST_DOMAIN_NOT_CONFIGURED;
		return -EINVAL;
	}

	id = args->port_id;

	port = dlb2_get_domain_used_dir_pq(id, vdev_req, domain);

	if (!port || !port->port_configured) {
		resp->status = DLB2_ST_INVALID_PORT_ID;
		return -EINVAL;
	}

	return 0;
}

static void dlb2_configure_domain_credits(struct dlb2_hw *hw,
					  struct dlb2_domain *domain)
{
	union dlb2_chp_cfg_ldb_vas_crd r0 = { {0} };
	union dlb2_chp_cfg_dir_vas_crd r1 = { {0} };

	r0.field.count = domain->num_ldb_credits;

	DLB2_CSR_WR(hw, DLB2_CHP_CFG_LDB_VAS_CRD(domain->id.phys_id), r0.val);

	r1.field.count = domain->num_dir_credits;

	DLB2_CSR_WR(hw, DLB2_CHP_CFG_DIR_VAS_CRD(domain->id.phys_id), r1.val);
}

static int
dlb2_domain_attach_resources(struct dlb2_hw *hw,
			     struct dlb2_function_resources *rsrcs,
			     struct dlb2_domain *domain,
			     struct dlb2_create_sched_domain_args *args,
			     struct dlb2_cmd_response *resp)
{
	int ret;

	ret = dlb2_attach_ldb_queues(hw,
				     rsrcs,
				     domain,
				     args->num_ldb_queues,
				     resp);
	if (ret < 0)
		return ret;

	ret = dlb2_attach_ldb_ports(hw,
				    rsrcs,
				    domain,
				    args,
				    resp);
	if (ret < 0)
		return ret;

	ret = dlb2_attach_dir_ports(hw,
				    rsrcs,
				    domain,
				    args->num_dir_ports,
				    resp);
	if (ret < 0)
		return ret;

	ret = dlb2_attach_ldb_credits(rsrcs,
				      domain,
				      args->num_ldb_credits,
				      resp);
	if (ret < 0)
		return ret;

	ret = dlb2_attach_dir_credits(rsrcs,
				      domain,
				      args->num_dir_credits,
				      resp);
	if (ret < 0)
		return ret;

	ret = dlb2_attach_domain_hist_list_entries(rsrcs,
						   domain,
						   args->num_hist_list_entries,
						   resp);
	if (ret < 0)
		return ret;

	ret = dlb2_attach_atomic_inflights(rsrcs,
					   domain,
					   args->num_atomic_inflights,
					   resp);
	if (ret < 0)
		return ret;

	dlb2_configure_domain_credits(hw, domain);

	domain->configured = true;

	domain->started = false;

	rsrcs->num_avail_domains--;

	return 0;
}

static int
dlb2_ldb_queue_attach_to_sn_group(struct dlb2_hw *hw,
				  struct dlb2_ldb_queue *queue,
				  struct dlb2_create_ldb_queue_args *args)
{
	int slot = -1;
	int i;

	queue->sn_cfg_valid = false;

	if (args->num_sequence_numbers == 0)
		return 0;

	for (i = 0; i < DLB2_MAX_NUM_SEQUENCE_NUMBER_GROUPS; i++) {
		struct dlb2_sn_group *group = &hw->rsrcs.sn_groups[i];

		if (group->sequence_numbers_per_queue ==
		    args->num_sequence_numbers &&
		    !dlb2_sn_group_full(group)) {
			slot = dlb2_sn_group_alloc_slot(group);
			if (slot >= 0)
				break;
		}
	}

	if (slot == -1) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: no sequence number slots available\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	queue->sn_cfg_valid = true;
	queue->sn_group = i;
	queue->sn_slot = slot;
	return 0;
}

static int
dlb2_ldb_queue_attach_resources(struct dlb2_hw *hw,
				struct dlb2_domain *domain,
				struct dlb2_ldb_queue *queue,
				struct dlb2_create_ldb_queue_args *args)
{
	int ret;

	ret = dlb2_ldb_queue_attach_to_sn_group(hw, queue, args);
	if (ret)
		return ret;

	/* Attach QID inflights */
	queue->num_qid_inflights = args->num_qid_inflights;

	/* Attach atomic inflights */
	queue->aqed_limit = args->num_atomic_inflights;

	domain->num_avail_aqed_entries -= args->num_atomic_inflights;
	domain->num_used_aqed_entries += args->num_atomic_inflights;

	return 0;
}

static void dlb2_ldb_port_cq_enable(struct dlb2_hw *hw,
				    struct dlb2_ldb_port *port)
{
	union dlb2_lsp_cq_ldb_dsbl reg;

	/* Don't re-enable the port if a removal is pending. The caller should
	 * mark this port as enabled (if it isn't already), and when the
	 * removal completes the port will be enabled.
	 */
	if (port->num_pending_removals)
		return;

	reg.field.disabled = 0;

	DLB2_CSR_WR(hw, DLB2_LSP_CQ_LDB_DSBL(port->id.phys_id), reg.val);

	dlb2_flush_csr(hw);
}

static void dlb2_ldb_port_cq_disable(struct dlb2_hw *hw,
				     struct dlb2_ldb_port *port)
{
	union dlb2_lsp_cq_ldb_dsbl reg;

	reg.field.disabled = 1;

	DLB2_CSR_WR(hw, DLB2_LSP_CQ_LDB_DSBL(port->id.phys_id), reg.val);

	dlb2_flush_csr(hw);
}

static void dlb2_dir_port_cq_enable(struct dlb2_hw *hw,
				    struct dlb2_dir_pq_pair *port)
{
	union dlb2_lsp_cq_dir_dsbl reg;

	reg.field.disabled = 0;

	DLB2_CSR_WR(hw, DLB2_LSP_CQ_DIR_DSBL(port->id.phys_id), reg.val);

	dlb2_flush_csr(hw);
}

static void dlb2_dir_port_cq_disable(struct dlb2_hw *hw,
				     struct dlb2_dir_pq_pair *port)
{
	union dlb2_lsp_cq_dir_dsbl reg;

	reg.field.disabled = 1;

	DLB2_CSR_WR(hw, DLB2_LSP_CQ_DIR_DSBL(port->id.phys_id), reg.val);

	dlb2_flush_csr(hw);
}

static void dlb2_ldb_port_configure_pp(struct dlb2_hw *hw,
				       struct dlb2_domain *domain,
				       struct dlb2_ldb_port *port,
				       bool vdev_req,
				       unsigned int vdev_id)
{
	union dlb2_sys_ldb_pp2vas r0 = { {0} };
	union dlb2_sys_ldb_pp_v r4 = { {0} };

	r0.field.vas = domain->id.phys_id;

	DLB2_CSR_WR(hw, DLB2_SYS_LDB_PP2VAS(port->id.phys_id), r0.val);

	if (vdev_req) {
		union dlb2_sys_vf_ldb_vpp2pp r1 = { {0} };
		union dlb2_sys_ldb_pp2vdev r2 = { {0} };
		union dlb2_sys_vf_ldb_vpp_v r3 = { {0} };
		unsigned int offs;
		u32 virt_id;

		/* DLB uses producer port address bits 17:12 to determine the
		 * producer port ID. In S-IOV mode, PP accesses come through
		 * the PF MMIO window for the physical producer port, so for
		 * translation purposes the virtual and physical port IDs are
		 * equal.
		 */
		if (hw->virt_mode == DLB2_VIRT_SRIOV)
			virt_id = port->id.virt_id;
		else
			virt_id = port->id.phys_id;

		r1.field.pp = port->id.phys_id;

		offs = vdev_id * DLB2_MAX_NUM_LDB_PORTS + virt_id;

		DLB2_CSR_WR(hw, DLB2_SYS_VF_LDB_VPP2PP(offs), r1.val);

		r2.field.vdev = vdev_id;

		DLB2_CSR_WR(hw,
			    DLB2_SYS_LDB_PP2VDEV(port->id.phys_id),
			    r2.val);

		r3.field.vpp_v = 1;

		DLB2_CSR_WR(hw, DLB2_SYS_VF_LDB_VPP_V(offs), r3.val);
	}

	r4.field.pp_v = 1;

	DLB2_CSR_WR(hw,
		    DLB2_SYS_LDB_PP_V(port->id.phys_id),
		    r4.val);
}

static int dlb2_ldb_port_configure_cq(struct dlb2_hw *hw,
				      struct dlb2_domain *domain,
				      struct dlb2_ldb_port *port,
				      uintptr_t cq_dma_base,
				      struct dlb2_create_ldb_port_args *args,
				      bool vdev_req,
				      unsigned int vdev_id)
{
	union dlb2_sys_ldb_cq_addr_l r0 = { {0} };
	union dlb2_sys_ldb_cq_addr_u r1 = { {0} };
	union dlb2_sys_ldb_cq2vf_pf_ro r2 = { {0} };
	union dlb2_chp_ldb_cq_tkn_depth_sel r3 = { {0} };
	union dlb2_lsp_cq_ldb_tkn_depth_sel r4 = { {0} };
	union dlb2_chp_hist_list_lim r5 = { {0} };
	union dlb2_chp_hist_list_base r6 = { {0} };
	union dlb2_lsp_cq_ldb_infl_lim r7 = { {0} };
	union dlb2_chp_hist_list_push_ptr r8 = { {0} };
	union dlb2_chp_hist_list_pop_ptr r9 = { {0} };
	union dlb2_sys_ldb_cq_at r10 = { {0} };
	union dlb2_sys_ldb_cq_pasid r11 = { {0} };
	union dlb2_chp_ldb_cq2vas r12 = { {0} };
	union dlb2_lsp_cq2priov r13 = { {0} };

	/* The CQ address is 64B-aligned, and the DLB only wants bits [63:6] */
	r0.field.addr_l = cq_dma_base >> 6;

	DLB2_CSR_WR(hw, DLB2_SYS_LDB_CQ_ADDR_L(port->id.phys_id), r0.val);

	r1.field.addr_u = cq_dma_base >> 32;

	DLB2_CSR_WR(hw, DLB2_SYS_LDB_CQ_ADDR_U(port->id.phys_id), r1.val);

	/* 'ro' == relaxed ordering. This setting allows DLB2 to write
	 * cache lines out-of-order (but QEs within a cache line are always
	 * updated in-order).
	 */
	r2.field.vf = vdev_id;
	r2.field.is_pf = !vdev_req && (hw->virt_mode != DLB2_VIRT_SIOV);
	r2.field.ro = 1;

	DLB2_CSR_WR(hw, DLB2_SYS_LDB_CQ2VF_PF_RO(port->id.phys_id), r2.val);

	if (args->cq_depth <= 8) {
		r3.field.token_depth_select = 1;
	} else if (args->cq_depth == 16) {
		r3.field.token_depth_select = 2;
	} else if (args->cq_depth == 32) {
		r3.field.token_depth_select = 3;
	} else if (args->cq_depth == 64) {
		r3.field.token_depth_select = 4;
	} else if (args->cq_depth == 128) {
		r3.field.token_depth_select = 5;
	} else if (args->cq_depth == 256) {
		r3.field.token_depth_select = 6;
	} else if (args->cq_depth == 512) {
		r3.field.token_depth_select = 7;
	} else if (args->cq_depth == 1024) {
		r3.field.token_depth_select = 8;
	} else {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: invalid CQ depth\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	DLB2_CSR_WR(hw,
		    DLB2_CHP_LDB_CQ_TKN_DEPTH_SEL(port->id.phys_id),
		    r3.val);

	/* To support CQs with depth less than 8, program the token count
	 * register with a non-zero initial value. Operations such as domain
	 * reset must take this initial value into account when quiescing the
	 * CQ.
	 */
	port->init_tkn_cnt = 0;

	if (args->cq_depth < 8) {
		union dlb2_lsp_cq_ldb_tkn_cnt r14 = { {0} };

		port->init_tkn_cnt = 8 - args->cq_depth;

		r14.field.token_count = port->init_tkn_cnt;

		DLB2_CSR_WR(hw,
			    DLB2_LSP_CQ_LDB_TKN_CNT(port->id.phys_id),
			    r14.val);
	} else {
		DLB2_CSR_WR(hw,
			    DLB2_LSP_CQ_LDB_TKN_CNT(port->id.phys_id),
			    DLB2_LSP_CQ_LDB_TKN_CNT_RST);
	}

	r4.field.token_depth_select = r3.field.token_depth_select;
	r4.field.ignore_depth = 0;

	DLB2_CSR_WR(hw,
		    DLB2_LSP_CQ_LDB_TKN_DEPTH_SEL(port->id.phys_id),
		    r4.val);

	/* Reset the CQ write pointer */
	DLB2_CSR_WR(hw,
		    DLB2_CHP_LDB_CQ_WPTR(port->id.phys_id),
		    DLB2_CHP_LDB_CQ_WPTR_RST);

	r5.field.limit = port->hist_list_entry_limit - 1;

	DLB2_CSR_WR(hw, DLB2_CHP_HIST_LIST_LIM(port->id.phys_id), r5.val);

	r6.field.base = port->hist_list_entry_base;

	DLB2_CSR_WR(hw, DLB2_CHP_HIST_LIST_BASE(port->id.phys_id), r6.val);

	/* The inflight limit sets a cap on the number of QEs for which this CQ
	 * can owe completions at one time.
	 */
	r7.field.limit = args->cq_history_list_size;

	DLB2_CSR_WR(hw, DLB2_LSP_CQ_LDB_INFL_LIM(port->id.phys_id), r7.val);

	r8.field.push_ptr = r6.field.base;
	r8.field.generation = 0;

	DLB2_CSR_WR(hw,
		    DLB2_CHP_HIST_LIST_PUSH_PTR(port->id.phys_id),
		    r8.val);

	r9.field.pop_ptr = r6.field.base;
	r9.field.generation = 0;

	DLB2_CSR_WR(hw, DLB2_CHP_HIST_LIST_POP_PTR(port->id.phys_id), r9.val);

	/* Address translation (AT) settings: 0: untranslated, 2: translated
	 * (see ATS spec regarding Address Type field for more details)
	 */
	r10.field.cq_at = 0;

	DLB2_CSR_WR(hw, DLB2_SYS_LDB_CQ_AT(port->id.phys_id), r10.val);

	if (vdev_req && hw->virt_mode == DLB2_VIRT_SIOV) {
		r11.field.pasid = hw->pasid[vdev_id];
		r11.field.fmt2 = 1;
	}

	DLB2_CSR_WR(hw,
		    DLB2_SYS_LDB_CQ_PASID(port->id.phys_id),
		    r11.val);

	r12.field.cq2vas = domain->id.phys_id;

	DLB2_CSR_WR(hw, DLB2_CHP_LDB_CQ2VAS(port->id.phys_id), r12.val);

	/* Disable the port's QID mappings */
	r13.field.v = 0;

	DLB2_CSR_WR(hw, DLB2_LSP_CQ2PRIOV(port->id.phys_id), r13.val);

	return 0;
}

static int dlb2_configure_ldb_port(struct dlb2_hw *hw,
				   struct dlb2_domain *domain,
				   struct dlb2_ldb_port *port,
				   uintptr_t cq_dma_base,
				   struct dlb2_create_ldb_port_args *args,
				   bool vdev_req,
				   unsigned int vdev_id)
{
	int ret, i;

	port->hist_list_entry_base = domain->hist_list_entry_base +
				     domain->hist_list_entry_offset;
	port->hist_list_entry_limit = port->hist_list_entry_base +
				      args->cq_history_list_size;

	domain->hist_list_entry_offset += args->cq_history_list_size;
	domain->avail_hist_list_entries -= args->cq_history_list_size;

	ret = dlb2_ldb_port_configure_cq(hw,
					 domain,
					 port,
					 cq_dma_base,
					 args,
					 vdev_req,
					 vdev_id);
	if (ret < 0)
		return ret;

	dlb2_ldb_port_configure_pp(hw,
				   domain,
				   port,
				   vdev_req,
				   vdev_id);

	dlb2_ldb_port_cq_enable(hw, port);

	for (i = 0; i < DLB2_MAX_NUM_QIDS_PER_LDB_CQ; i++)
		port->qid_map[i].state = DLB2_QUEUE_UNMAPPED;
	port->num_mappings = 0;

	port->enabled = true;

	port->configured = true;

	return 0;
}

static void dlb2_dir_port_configure_pp(struct dlb2_hw *hw,
				       struct dlb2_domain *domain,
				       struct dlb2_dir_pq_pair *port,
				       bool vdev_req,
				       unsigned int vdev_id)
{
	union dlb2_sys_dir_pp2vas r0 = { {0} };
	union dlb2_sys_dir_pp_v r4 = { {0} };

	r0.field.vas = domain->id.phys_id;

	DLB2_CSR_WR(hw, DLB2_SYS_DIR_PP2VAS(port->id.phys_id), r0.val);

	if (vdev_req) {
		union dlb2_sys_vf_dir_vpp2pp r1 = { {0} };
		union dlb2_sys_dir_pp2vdev r2 = { {0} };
		union dlb2_sys_vf_dir_vpp_v r3 = { {0} };
		unsigned int offs;
		u32 virt_id;

		/* DLB uses producer port address bits 17:12 to determine the
		 * producer port ID. In S-IOV mode, PP accesses come through
		 * the PF MMIO window for the physical producer port, so for
		 * translation purposes the virtual and physical port IDs are
		 * equal.
		 */
		if (hw->virt_mode == DLB2_VIRT_SRIOV)
			virt_id = port->id.virt_id;
		else
			virt_id = port->id.phys_id;

		r1.field.pp = port->id.phys_id;

		offs = vdev_id * DLB2_MAX_NUM_DIR_PORTS + virt_id;

		DLB2_CSR_WR(hw, DLB2_SYS_VF_DIR_VPP2PP(offs), r1.val);

		r2.field.vdev = vdev_id;

		DLB2_CSR_WR(hw,
			    DLB2_SYS_DIR_PP2VDEV(port->id.phys_id),
			    r2.val);

		r3.field.vpp_v = 1;

		DLB2_CSR_WR(hw, DLB2_SYS_VF_DIR_VPP_V(offs), r3.val);
	}

	r4.field.pp_v = 1;

	DLB2_CSR_WR(hw,
		    DLB2_SYS_DIR_PP_V(port->id.phys_id),
		    r4.val);
}

static int dlb2_dir_port_configure_cq(struct dlb2_hw *hw,
				      struct dlb2_domain *domain,
				      struct dlb2_dir_pq_pair *port,
				      uintptr_t cq_dma_base,
				      struct dlb2_create_dir_port_args *args,
				      bool vdev_req,
				      unsigned int vdev_id)
{
	union dlb2_sys_dir_cq_addr_l r0 = { {0} };
	union dlb2_sys_dir_cq_addr_u r1 = { {0} };
	union dlb2_sys_dir_cq2vf_pf_ro r2 = { {0} };
	union dlb2_chp_dir_cq_tkn_depth_sel r3 = { {0} };
	union dlb2_lsp_cq_dir_tkn_depth_sel_dsi r4 = { {0} };
	union dlb2_sys_dir_cq_fmt r9 = { {0} };
	union dlb2_sys_dir_cq_at r10 = { {0} };
	union dlb2_sys_dir_cq_pasid r11 = { {0} };
	union dlb2_chp_dir_cq2vas r12 = { {0} };

	/* The CQ address is 64B-aligned, and the DLB only wants bits [63:6] */
	r0.field.addr_l = cq_dma_base >> 6;

	DLB2_CSR_WR(hw, DLB2_SYS_DIR_CQ_ADDR_L(port->id.phys_id), r0.val);

	r1.field.addr_u = cq_dma_base >> 32;

	DLB2_CSR_WR(hw, DLB2_SYS_DIR_CQ_ADDR_U(port->id.phys_id), r1.val);

	/* 'ro' == relaxed ordering. This setting allows DLB2 to write
	 * cache lines out-of-order (but QEs within a cache line are always
	 * updated in-order).
	 */
	r2.field.vf = vdev_id;
	r2.field.is_pf = !vdev_req && (hw->virt_mode != DLB2_VIRT_SIOV);
	r2.field.ro = 1;

	DLB2_CSR_WR(hw, DLB2_SYS_DIR_CQ2VF_PF_RO(port->id.phys_id), r2.val);

	if (args->cq_depth <= 8) {
		r3.field.token_depth_select = 1;
	} else if (args->cq_depth == 16) {
		r3.field.token_depth_select = 2;
	} else if (args->cq_depth == 32) {
		r3.field.token_depth_select = 3;
	} else if (args->cq_depth == 64) {
		r3.field.token_depth_select = 4;
	} else if (args->cq_depth == 128) {
		r3.field.token_depth_select = 5;
	} else if (args->cq_depth == 256) {
		r3.field.token_depth_select = 6;
	} else if (args->cq_depth == 512) {
		r3.field.token_depth_select = 7;
	} else if (args->cq_depth == 1024) {
		r3.field.token_depth_select = 8;
	} else {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: invalid CQ depth\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	DLB2_CSR_WR(hw,
		    DLB2_CHP_DIR_CQ_TKN_DEPTH_SEL(port->id.phys_id),
		    r3.val);

	/* To support CQs with depth less than 8, program the token count
	 * register with a non-zero initial value. Operations such as domain
	 * reset must take this initial value into account when quiescing the
	 * CQ.
	 */
	port->init_tkn_cnt = 0;

	if (args->cq_depth < 8) {
		union dlb2_lsp_cq_dir_tkn_cnt r13 = { {0} };

		port->init_tkn_cnt = 8 - args->cq_depth;

		r13.field.count = port->init_tkn_cnt;

		DLB2_CSR_WR(hw,
			    DLB2_LSP_CQ_DIR_TKN_CNT(port->id.phys_id),
			    r13.val);
	} else {
		DLB2_CSR_WR(hw,
			    DLB2_LSP_CQ_DIR_TKN_CNT(port->id.phys_id),
			    DLB2_LSP_CQ_DIR_TKN_CNT_RST);
	}

	r4.field.token_depth_select = r3.field.token_depth_select;
	r4.field.disable_wb_opt = 0;
	r4.field.ignore_depth = 0;

	DLB2_CSR_WR(hw,
		    DLB2_LSP_CQ_DIR_TKN_DEPTH_SEL_DSI(port->id.phys_id),
		    r4.val);

	/* Reset the CQ write pointer */
	DLB2_CSR_WR(hw,
		    DLB2_CHP_DIR_CQ_WPTR(port->id.phys_id),
		    DLB2_CHP_DIR_CQ_WPTR_RST);

	/* Virtualize the PPID */
	r9.field.keep_pf_ppid = 0;

	DLB2_CSR_WR(hw, DLB2_SYS_DIR_CQ_FMT(port->id.phys_id), r9.val);

	/* Address translation (AT) settings: 0: untranslated, 2: translated
	 * (see ATS spec regarding Address Type field for more details)
	 */
	r10.field.cq_at = 0;

	DLB2_CSR_WR(hw, DLB2_SYS_DIR_CQ_AT(port->id.phys_id), r10.val);

	if (vdev_req && hw->virt_mode == DLB2_VIRT_SIOV) {
		r11.field.pasid = hw->pasid[vdev_id];
		r11.field.fmt2 = 1;
	}

	DLB2_CSR_WR(hw,
		    DLB2_SYS_DIR_CQ_PASID(port->id.phys_id),
		    r11.val);

	r12.field.cq2vas = domain->id.phys_id;

	DLB2_CSR_WR(hw, DLB2_CHP_DIR_CQ2VAS(port->id.phys_id), r12.val);

	return 0;
}

static int dlb2_configure_dir_port(struct dlb2_hw *hw,
				   struct dlb2_domain *domain,
				   struct dlb2_dir_pq_pair *port,
				   uintptr_t cq_dma_base,
				   struct dlb2_create_dir_port_args *args,
				   bool vdev_req,
				   unsigned int vdev_id)
{
	int ret;

	ret = dlb2_dir_port_configure_cq(hw,
					 domain,
					 port,
					 cq_dma_base,
					 args,
					 vdev_req,
					 vdev_id);

	if (ret < 0)
		return ret;

	dlb2_dir_port_configure_pp(hw,
				   domain,
				   port,
				   vdev_req,
				   vdev_id);

	dlb2_dir_port_cq_enable(hw, port);

	port->enabled = true;

	port->port_configured = true;

	return 0;
}

static int dlb2_ldb_port_map_qid_static(struct dlb2_hw *hw,
					struct dlb2_ldb_port *p,
					struct dlb2_ldb_queue *q,
					u8 priority)
{
	union dlb2_lsp_cq2priov r0;
	union dlb2_lsp_cq2qid0 r1;
	union dlb2_atm_qid2cqidix_00 r2;
	union dlb2_lsp_qid2cqidix_00 r3;
	union dlb2_lsp_qid2cqidix2_00 r4;
	enum dlb2_qid_map_state state;
	int i;

	/* Look for a pending or already mapped slot, else an unused slot */
	if (!dlb2_port_find_slot_queue(p, DLB2_QUEUE_MAP_IN_PROG, q, &i) &&
	    !dlb2_port_find_slot_queue(p, DLB2_QUEUE_MAPPED, q, &i) &&
	    !dlb2_port_find_slot(p, DLB2_QUEUE_UNMAPPED, &i)) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: CQ has no available QID mapping slots\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	if (i >= DLB2_MAX_NUM_QIDS_PER_LDB_CQ) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: port slot tracking failed\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	/* Read-modify-write the priority and valid bit register */
	r0.val = DLB2_CSR_RD(hw, DLB2_LSP_CQ2PRIOV(p->id.phys_id));

	r0.field.v |= 1 << i;
	r0.field.prio |= (priority & 0x7) << i * 3;

	DLB2_CSR_WR(hw, DLB2_LSP_CQ2PRIOV(p->id.phys_id), r0.val);

	/* Read-modify-write the QID map register */
	if (i < 4)
		r1.val = DLB2_CSR_RD(hw, DLB2_LSP_CQ2QID0(p->id.phys_id));
	else
		r1.val = DLB2_CSR_RD(hw, DLB2_LSP_CQ2QID1(p->id.phys_id));

	if (i == 0 || i == 4)
		r1.field.qid_p0 = q->id.phys_id;
	if (i == 1 || i == 5)
		r1.field.qid_p1 = q->id.phys_id;
	if (i == 2 || i == 6)
		r1.field.qid_p2 = q->id.phys_id;
	if (i == 3 || i == 7)
		r1.field.qid_p3 = q->id.phys_id;

	if (i < 4)
		DLB2_CSR_WR(hw, DLB2_LSP_CQ2QID0(p->id.phys_id), r1.val);
	else
		DLB2_CSR_WR(hw, DLB2_LSP_CQ2QID1(p->id.phys_id), r1.val);

	r2.val = DLB2_CSR_RD(hw,
			     DLB2_ATM_QID2CQIDIX(q->id.phys_id,
						 p->id.phys_id / 4));

	r3.val = DLB2_CSR_RD(hw,
			     DLB2_LSP_QID2CQIDIX(q->id.phys_id,
						 p->id.phys_id / 4));

	r4.val = DLB2_CSR_RD(hw,
			     DLB2_LSP_QID2CQIDIX2(q->id.phys_id,
						  p->id.phys_id / 4));

	switch (p->id.phys_id % 4) {
	case 0:
		r2.field.cq_p0 |= 1 << i;
		r3.field.cq_p0 |= 1 << i;
		r4.field.cq_p0 |= 1 << i;
		break;

	case 1:
		r2.field.cq_p1 |= 1 << i;
		r3.field.cq_p1 |= 1 << i;
		r4.field.cq_p1 |= 1 << i;
		break;

	case 2:
		r2.field.cq_p2 |= 1 << i;
		r3.field.cq_p2 |= 1 << i;
		r4.field.cq_p2 |= 1 << i;
		break;

	case 3:
		r2.field.cq_p3 |= 1 << i;
		r3.field.cq_p3 |= 1 << i;
		r4.field.cq_p3 |= 1 << i;
		break;
	}

	DLB2_CSR_WR(hw,
		    DLB2_ATM_QID2CQIDIX(q->id.phys_id, p->id.phys_id / 4),
		    r2.val);

	DLB2_CSR_WR(hw,
		    DLB2_LSP_QID2CQIDIX(q->id.phys_id, p->id.phys_id / 4),
		    r3.val);

	DLB2_CSR_WR(hw,
		    DLB2_LSP_QID2CQIDIX2(q->id.phys_id, p->id.phys_id / 4),
		    r4.val);

	dlb2_flush_csr(hw);

	p->qid_map[i].qid = q->id.phys_id;
	p->qid_map[i].priority = priority;

	state = DLB2_QUEUE_MAPPED;

	return dlb2_port_slot_state_transition(hw, p, q, i, state);
}

static void dlb2_ldb_port_change_qid_priority(struct dlb2_hw *hw,
					      struct dlb2_ldb_port *port,
					      int slot,
					      struct dlb2_map_qid_args *args)
{
	union dlb2_lsp_cq2priov r0;

	/* Read-modify-write the priority and valid bit register */
	r0.val = DLB2_CSR_RD(hw, DLB2_LSP_CQ2PRIOV(port->id.phys_id));

	r0.field.v |= 1 << slot;
	r0.field.prio |= (args->priority & 0x7) << slot * 3;

	DLB2_CSR_WR(hw, DLB2_LSP_CQ2PRIOV(port->id.phys_id), r0.val);

	dlb2_flush_csr(hw);

	port->qid_map[slot].priority = args->priority;
}

static int dlb2_ldb_port_set_has_work_bits(struct dlb2_hw *hw,
					   struct dlb2_ldb_port *port,
					   struct dlb2_ldb_queue *queue,
					   int slot)
{
	union dlb2_lsp_qid_aqed_active_cnt r0;
	union dlb2_lsp_qid_ldb_enqueue_cnt r1;
	union dlb2_lsp_ldb_sched_ctrl r2 = { {0} };

	/* Set the atomic scheduling haswork bit */
	r0.val = DLB2_CSR_RD(hw,
			     DLB2_LSP_QID_AQED_ACTIVE_CNT(queue->id.phys_id));

	r2.field.cq = port->id.phys_id;
	r2.field.qidix = slot;
	r2.field.value = 1;
	r2.field.rlist_haswork_v = r0.field.count > 0;

	/* Set the non-atomic scheduling haswork bit */
	DLB2_CSR_WR(hw, DLB2_LSP_LDB_SCHED_CTRL, r2.val);

	r1.val = DLB2_CSR_RD(hw,
			     DLB2_LSP_QID_LDB_ENQUEUE_CNT(queue->id.phys_id));

	memset(&r2, 0, sizeof(r2));

	r2.field.cq = port->id.phys_id;
	r2.field.qidix = slot;
	r2.field.value = 1;
	r2.field.nalb_haswork_v = (r1.field.count > 0);

	DLB2_CSR_WR(hw, DLB2_LSP_LDB_SCHED_CTRL, r2.val);

	dlb2_flush_csr(hw);

	return 0;
}

static void dlb2_ldb_port_clear_has_work_bits(struct dlb2_hw *hw,
					      struct dlb2_ldb_port *port,
					      u8 slot)
{
	union dlb2_lsp_ldb_sched_ctrl r2 = { {0} };

	r2.field.cq = port->id.phys_id;
	r2.field.qidix = slot;
	r2.field.value = 0;
	r2.field.rlist_haswork_v = 1;

	DLB2_CSR_WR(hw, DLB2_LSP_LDB_SCHED_CTRL, r2.val);

	memset(&r2, 0, sizeof(r2));

	r2.field.cq = port->id.phys_id;
	r2.field.qidix = slot;
	r2.field.value = 0;
	r2.field.nalb_haswork_v = 1;

	DLB2_CSR_WR(hw, DLB2_LSP_LDB_SCHED_CTRL, r2.val);

	dlb2_flush_csr(hw);
}

static void dlb2_ldb_port_clear_queue_if_status(struct dlb2_hw *hw,
						struct dlb2_ldb_port *port,
						int slot)
{
	union dlb2_lsp_ldb_sched_ctrl r0 = { {0} };

	r0.field.cq = port->id.phys_id;
	r0.field.qidix = slot;
	r0.field.value = 0;
	r0.field.inflight_ok_v = 1;

	DLB2_CSR_WR(hw, DLB2_LSP_LDB_SCHED_CTRL, r0.val);

	dlb2_flush_csr(hw);
}

static void dlb2_ldb_port_set_queue_if_status(struct dlb2_hw *hw,
					      struct dlb2_ldb_port *port,
					      int slot)
{
	union dlb2_lsp_ldb_sched_ctrl r0 = { {0} };

	r0.field.cq = port->id.phys_id;
	r0.field.qidix = slot;
	r0.field.value = 1;
	r0.field.inflight_ok_v = 1;

	DLB2_CSR_WR(hw, DLB2_LSP_LDB_SCHED_CTRL, r0.val);

	dlb2_flush_csr(hw);
}

static void dlb2_ldb_queue_set_inflight_limit(struct dlb2_hw *hw,
					      struct dlb2_ldb_queue *queue)
{
	union dlb2_lsp_qid_ldb_infl_lim r0 = { {0} };

	r0.field.limit = queue->num_qid_inflights;

	DLB2_CSR_WR(hw, DLB2_LSP_QID_LDB_INFL_LIM(queue->id.phys_id), r0.val);
}

static void dlb2_ldb_queue_clear_inflight_limit(struct dlb2_hw *hw,
						struct dlb2_ldb_queue *queue)
{
	DLB2_CSR_WR(hw,
		    DLB2_LSP_QID_LDB_INFL_LIM(queue->id.phys_id),
		    DLB2_LSP_QID_LDB_INFL_LIM_RST);
}

/* dlb2_ldb_queue_{enable, disable}_mapped_cqs() don't operate exactly as
 * their function names imply, and should only be called by the dynamic CQ
 * mapping code.
 */
static void dlb2_ldb_queue_disable_mapped_cqs(struct dlb2_hw *hw,
					      struct dlb2_domain *domain,
					      struct dlb2_ldb_queue *queue)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_ldb_port *port;
	int slot, i;

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i], port, iter) {
			enum dlb2_qid_map_state state = DLB2_QUEUE_MAPPED;

			if (!dlb2_port_find_slot_queue(port, state,
						       queue, &slot))
				continue;

			if (port->enabled)
				dlb2_ldb_port_cq_disable(hw, port);
		}
	}
}

static void dlb2_ldb_queue_enable_mapped_cqs(struct dlb2_hw *hw,
					     struct dlb2_domain *domain,
					     struct dlb2_ldb_queue *queue)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_ldb_port *port;
	int slot, i;

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i], port, iter) {
			enum dlb2_qid_map_state state = DLB2_QUEUE_MAPPED;

			if (!dlb2_port_find_slot_queue(port, state,
						       queue, &slot))
				continue;

			if (port->enabled)
				dlb2_ldb_port_cq_enable(hw, port);
		}
	}
}

static int dlb2_ldb_port_finish_map_qid_dynamic(struct dlb2_hw *hw,
						struct dlb2_domain *domain,
						struct dlb2_ldb_port *port,
						struct dlb2_ldb_queue *queue)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	union dlb2_lsp_qid_ldb_infl_cnt r0;
	enum dlb2_qid_map_state state;
	int slot, ret, i;
	u8 prio;

	r0.val = DLB2_CSR_RD(hw,
			     DLB2_LSP_QID_LDB_INFL_CNT(queue->id.phys_id));

	if (r0.field.count) {
		DLB2_BASE_ERR(hw,
			      "[%s()] Internal error: non-zero QID inflight count\n",
			      __func__);
		return -EINVAL;
	}

	/* Static map the port and set its corresponding has_work bits.
	 */
	state = DLB2_QUEUE_MAP_IN_PROG;
	if (!dlb2_port_find_slot_queue(port, state, queue, &slot))
		return -EINVAL;

	if (slot >= DLB2_MAX_NUM_QIDS_PER_LDB_CQ) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: port slot tracking failed\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	prio = port->qid_map[slot].priority;

	/* Update the CQ2QID, CQ2PRIOV, and QID2CQIDX registers, and
	 * the port's qid_map state.
	 */
	ret = dlb2_ldb_port_map_qid_static(hw, port, queue, prio);
	if (ret)
		return ret;

	ret = dlb2_ldb_port_set_has_work_bits(hw, port, queue, slot);
	if (ret)
		return ret;

	/* Ensure IF_status(cq,qid) is 0 before enabling the port to
	 * prevent spurious schedules to cause the queue's inflight
	 * count to increase.
	 */
	dlb2_ldb_port_clear_queue_if_status(hw, port, slot);

	/* Reset the queue's inflight status */
	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i], port, iter) {
			state = DLB2_QUEUE_MAPPED;
			if (!dlb2_port_find_slot_queue(port, state,
						       queue, &slot))
				continue;

			dlb2_ldb_port_set_queue_if_status(hw, port, slot);
		}
	}

	dlb2_ldb_queue_set_inflight_limit(hw, queue);

	/* Re-enable CQs mapped to this queue */
	dlb2_ldb_queue_enable_mapped_cqs(hw, domain, queue);

	/* If this queue has other mappings pending, clear its inflight limit */
	if (queue->num_pending_additions > 0)
		dlb2_ldb_queue_clear_inflight_limit(hw, queue);

	return 0;
}

/**
 * dlb2_ldb_port_map_qid_dynamic() - perform a "dynamic" QID->CQ mapping
 * @hw: dlb2_hw handle for a particular device.
 * @port: load-balanced port
 * @queue: load-balanced queue
 * @priority: queue servicing priority
 *
 * Returns 0 if the queue was mapped, 1 if the mapping is scheduled to occur
 * at a later point, and <0 if an error occurred.
 */
static int dlb2_ldb_port_map_qid_dynamic(struct dlb2_hw *hw,
					 struct dlb2_ldb_port *port,
					 struct dlb2_ldb_queue *queue,
					 u8 priority)
{
	union dlb2_lsp_qid_ldb_infl_cnt r0 = { {0} };
	enum dlb2_qid_map_state state;
	struct dlb2_domain *domain;
	int domain_id, slot, ret;

	domain_id = port->domain_id.phys_id;

	domain = dlb2_get_domain_from_id(hw, domain_id, false, 0);
	if (!domain) {
		DLB2_BASE_ERR(hw,
			      "[%s()] Internal error: unable to find domain %d\n",
			      __func__, port->domain_id.phys_id);
		return -EINVAL;
	}

	/* Set the QID inflight limit to 0 to prevent further scheduling of the
	 * queue.
	 */
	DLB2_CSR_WR(hw, DLB2_LSP_QID_LDB_INFL_LIM(queue->id.phys_id), 0);

	if (!dlb2_port_find_slot(port, DLB2_QUEUE_UNMAPPED, &slot)) {
		DLB2_BASE_ERR(hw,
			      "Internal error: No available unmapped slots\n");
		return -EFAULT;
	}

	if (slot >= DLB2_MAX_NUM_QIDS_PER_LDB_CQ) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: port slot tracking failed\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	port->qid_map[slot].qid = queue->id.phys_id;
	port->qid_map[slot].priority = priority;

	state = DLB2_QUEUE_MAP_IN_PROG;
	ret = dlb2_port_slot_state_transition(hw, port, queue, slot, state);
	if (ret)
		return ret;

	r0.val = DLB2_CSR_RD(hw,
			     DLB2_LSP_QID_LDB_INFL_CNT(queue->id.phys_id));

	if (r0.field.count) {
		/* The queue is owed completions so it's not safe to map it
		 * yet. Schedule a kernel thread to complete the mapping later,
		 * once software has completed all the queue's inflight events.
		 */
		if (!os_worker_active(hw))
			os_schedule_work(hw);

		return 1;
	}

	/* Disable the affected CQ, and the CQs already mapped to the QID,
	 * before reading the QID's inflight count a second time. There is an
	 * unlikely race in which the QID may schedule one more QE after we
	 * read an inflight count of 0, and disabling the CQs guarantees that
	 * the race will not occur after a re-read of the inflight count
	 * register.
	 */
	if (port->enabled)
		dlb2_ldb_port_cq_disable(hw, port);

	dlb2_ldb_queue_disable_mapped_cqs(hw, domain, queue);

	r0.val = DLB2_CSR_RD(hw,
			     DLB2_LSP_QID_LDB_INFL_CNT(queue->id.phys_id));

	if (r0.field.count) {
		if (port->enabled)
			dlb2_ldb_port_cq_enable(hw, port);

		dlb2_ldb_queue_enable_mapped_cqs(hw, domain, queue);

		/* The queue is owed completions so it's not safe to map it
		 * yet. Schedule a kernel thread to complete the mapping later,
		 * once software has completed all the queue's inflight events.
		 */
		if (!os_worker_active(hw))
			os_schedule_work(hw);

		return 1;
	}

	return dlb2_ldb_port_finish_map_qid_dynamic(hw, domain, port, queue);
}

static int dlb2_ldb_port_map_qid(struct dlb2_hw *hw,
				 struct dlb2_domain *domain,
				 struct dlb2_ldb_port *port,
				 struct dlb2_ldb_queue *queue,
				 u8 prio)
{
	if (domain->started)
		return dlb2_ldb_port_map_qid_dynamic(hw, port, queue, prio);
	else
		return dlb2_ldb_port_map_qid_static(hw, port, queue, prio);
}

static int dlb2_ldb_port_unmap_qid(struct dlb2_hw *hw,
				   struct dlb2_ldb_port *port,
				   struct dlb2_ldb_queue *queue)
{
	enum dlb2_qid_map_state mapped, in_progress, pending_map, unmapped;
	union dlb2_lsp_cq2priov r0;
	union dlb2_atm_qid2cqidix_00 r1;
	union dlb2_lsp_qid2cqidix_00 r2;
	union dlb2_lsp_qid2cqidix2_00 r3;
	u32 queue_id;
	u32 port_id;
	int i;

	/* Find the queue's slot */
	mapped = DLB2_QUEUE_MAPPED;
	in_progress = DLB2_QUEUE_UNMAP_IN_PROG;
	pending_map = DLB2_QUEUE_UNMAP_IN_PROG_PENDING_MAP;

	if (!dlb2_port_find_slot_queue(port, mapped, queue, &i) &&
	    !dlb2_port_find_slot_queue(port, in_progress, queue, &i) &&
	    !dlb2_port_find_slot_queue(port, pending_map, queue, &i)) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: QID %d isn't mapped\n",
			      __func__, __LINE__, queue->id.phys_id);
		return -EFAULT;
	}

	if (i >= DLB2_MAX_NUM_QIDS_PER_LDB_CQ) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: port slot tracking failed\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	port_id = port->id.phys_id;
	queue_id = queue->id.phys_id;

	/* Read-modify-write the priority and valid bit register */
	r0.val = DLB2_CSR_RD(hw, DLB2_LSP_CQ2PRIOV(port_id));

	r0.field.v &= ~(1 << i);

	DLB2_CSR_WR(hw, DLB2_LSP_CQ2PRIOV(port_id), r0.val);

	r1.val = DLB2_CSR_RD(hw,
			     DLB2_ATM_QID2CQIDIX(queue_id, port_id / 4));

	r2.val = DLB2_CSR_RD(hw,
			     DLB2_LSP_QID2CQIDIX(queue_id, port_id / 4));

	r3.val = DLB2_CSR_RD(hw,
			     DLB2_LSP_QID2CQIDIX2(queue_id, port_id / 4));

	switch (port_id % 4) {
	case 0:
		r1.field.cq_p0 &= ~(1 << i);
		r2.field.cq_p0 &= ~(1 << i);
		r3.field.cq_p0 &= ~(1 << i);
		break;

	case 1:
		r1.field.cq_p1 &= ~(1 << i);
		r2.field.cq_p1 &= ~(1 << i);
		r3.field.cq_p1 &= ~(1 << i);
		break;

	case 2:
		r1.field.cq_p2 &= ~(1 << i);
		r2.field.cq_p2 &= ~(1 << i);
		r3.field.cq_p2 &= ~(1 << i);
		break;

	case 3:
		r1.field.cq_p3 &= ~(1 << i);
		r2.field.cq_p3 &= ~(1 << i);
		r3.field.cq_p3 &= ~(1 << i);
		break;
	}

	DLB2_CSR_WR(hw,
		    DLB2_ATM_QID2CQIDIX(queue_id, port_id / 4),
		    r1.val);

	DLB2_CSR_WR(hw,
		    DLB2_LSP_QID2CQIDIX(queue_id, port_id / 4),
		    r2.val);

	DLB2_CSR_WR(hw,
		    DLB2_LSP_QID2CQIDIX2(queue_id, port_id / 4),
		    r3.val);

	dlb2_flush_csr(hw);

	unmapped = DLB2_QUEUE_UNMAPPED;

	return dlb2_port_slot_state_transition(hw, port, queue, i, unmapped);
}

static void
dlb2_log_create_sched_domain_args(struct dlb2_hw *hw,
				  struct dlb2_create_sched_domain_args *args,
				  bool vdev_req,
				  unsigned int vdev_id)
{
	DLB2_BASE_DBG(hw, "DLB2 create sched domain arguments:\n");
	if (vdev_req)
		DLB2_BASE_DBG(hw, "(Request from vdev %d)\n", vdev_id);
	DLB2_BASE_DBG(hw, "\tNumber of LDB queues:          %d\n",
		      args->num_ldb_queues);
	DLB2_BASE_DBG(hw, "\tNumber of LDB ports (any CoS): %d\n",
		      args->num_ldb_ports);
	DLB2_BASE_DBG(hw, "\tNumber of LDB ports (CoS 0):   %d\n",
		      args->num_cos0_ldb_ports);
	DLB2_BASE_DBG(hw, "\tNumber of LDB ports (CoS 1):   %d\n",
		      args->num_cos1_ldb_ports);
	DLB2_BASE_DBG(hw, "\tNumber of LDB ports (CoS 2):   %d\n",
		      args->num_cos2_ldb_ports);
	DLB2_BASE_DBG(hw, "\tNumber of LDB ports (CoS 3):   %d\n",
		      args->num_cos3_ldb_ports);
	DLB2_BASE_DBG(hw, "\tStrict CoS allocation:         %d\n",
		      args->cos_strict);
	DLB2_BASE_DBG(hw, "\tNumber of DIR ports:           %d\n",
		      args->num_dir_ports);
	DLB2_BASE_DBG(hw, "\tNumber of ATM inflights:       %d\n",
		      args->num_atomic_inflights);
	DLB2_BASE_DBG(hw, "\tNumber of hist list entries:   %d\n",
		      args->num_hist_list_entries);
	DLB2_BASE_DBG(hw, "\tNumber of LDB credits:         %d\n",
		      args->num_ldb_credits);
	DLB2_BASE_DBG(hw, "\tNumber of DIR credits:         %d\n",
		      args->num_dir_credits);
}

/**
 * dlb2_hw_create_sched_domain() - Allocate and initialize a DLB scheduling
 *	domain and its resources.
 * @hw:	  Contains the current state of the DLB2 hardware.
 * @args: User-provided arguments.
 * @resp: Response to user.
 *
 * Return: returns < 0 on error, 0 otherwise. If the driver is unable to
 * satisfy a request, resp->status will be set accordingly.
 */
int dlb2_hw_create_sched_domain(struct dlb2_hw *hw,
				struct dlb2_create_sched_domain_args *args,
				struct dlb2_cmd_response *resp,
				bool vdev_req,
				unsigned int vdev_id)
{
	struct dlb2_domain *domain;
	struct dlb2_function_resources *rsrcs;
	int ret;

	rsrcs = (vdev_req) ? &hw->vdev[vdev_id] : &hw->pf;

	dlb2_log_create_sched_domain_args(hw, args, vdev_req, vdev_id);

	/* Verify that hardware resources are available before attempting to
	 * satisfy the request. This simplifies the error unwinding code.
	 */
	ret = dlb2_verify_create_sched_dom_args(rsrcs, args, resp);
	if (ret)
		return ret;

	domain = DLB2_FUNC_LIST_HEAD(rsrcs->avail_domains, typeof(*domain));

	/* Verification should catch this. */
	if (!domain) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: no available domains\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	if (domain->configured) {
		DLB2_BASE_ERR(hw,
			      "[%s()] Internal error: avail_domains contains configured domains.\n",
			      __func__);
		return -EFAULT;
	}

	dlb2_init_domain_rsrc_lists(domain);

	/* Verification should catch this too. */
	ret = dlb2_domain_attach_resources(hw, rsrcs, domain, args, resp);
	if (ret < 0) {
		DLB2_BASE_ERR(hw,
			      "[%s()] Internal error: failed to verify args.\n",
			      __func__);

		return ret;
	}

	dlb2_list_del(&rsrcs->avail_domains, &domain->func_list);

	dlb2_list_add(&rsrcs->used_domains, &domain->func_list);

	resp->id = (vdev_req) ? domain->id.virt_id : domain->id.phys_id;
	resp->status = 0;

	return 0;
}

static void
dlb2_log_create_ldb_queue_args(struct dlb2_hw *hw,
			       u32 domain_id,
			       struct dlb2_create_ldb_queue_args *args,
			       bool vdev_req,
			       unsigned int vdev_id)
{
	DLB2_BASE_DBG(hw, "DLB2 create load-balanced queue arguments:\n");
	if (vdev_req)
		DLB2_BASE_DBG(hw, "(Request from vdev %d)\n", vdev_id);
	DLB2_BASE_DBG(hw, "\tDomain ID:                  %d\n",
		      domain_id);
	DLB2_BASE_DBG(hw, "\tNumber of sequence numbers: %d\n",
		      args->num_sequence_numbers);
	DLB2_BASE_DBG(hw, "\tNumber of QID inflights:    %d\n",
		      args->num_qid_inflights);
	DLB2_BASE_DBG(hw, "\tNumber of ATM inflights:    %d\n",
		      args->num_atomic_inflights);
}

/**
 * dlb2_hw_create_ldb_queue() - Allocate and initialize a DLB LDB queue.
 * @hw:	  Contains the current state of the DLB2 hardware.
 * @args: User-provided arguments.
 * @resp: Response to user.
 *
 * Return: returns < 0 on error, 0 otherwise. If the driver is unable to
 * satisfy a request, resp->status will be set accordingly.
 */
int dlb2_hw_create_ldb_queue(struct dlb2_hw *hw,
			     u32 domain_id,
			     struct dlb2_create_ldb_queue_args *args,
			     struct dlb2_cmd_response *resp,
			     bool vdev_req,
			     unsigned int vdev_id)
{
	struct dlb2_ldb_queue *queue;
	struct dlb2_domain *domain;
	int ret;

	dlb2_log_create_ldb_queue_args(hw, domain_id, args, vdev_req, vdev_id);

	/* Verify that hardware resources are available before attempting to
	 * satisfy the request. This simplifies the error unwinding code.
	 */
	ret = dlb2_verify_create_ldb_queue_args(hw,
						domain_id,
						args,
						resp,
						vdev_req,
						vdev_id);
	if (ret)
		return ret;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);
	if (!domain) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: domain not found\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	queue = DLB2_DOM_LIST_HEAD(domain->avail_ldb_queues, typeof(*queue));

	/* Verification should catch this. */
	if (!queue) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: no available ldb queues\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	ret = dlb2_ldb_queue_attach_resources(hw, domain, queue, args);
	if (ret < 0) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: failed to attach the ldb queue resources\n",
			      __func__, __LINE__);
		return ret;
	}

	dlb2_configure_ldb_queue(hw, domain, queue, args, vdev_req, vdev_id);

	queue->num_mappings = 0;

	queue->configured = true;

	/* Configuration succeeded, so move the resource from the 'avail' to
	 * the 'used' list.
	 */
	dlb2_list_del(&domain->avail_ldb_queues, &queue->domain_list);

	dlb2_list_add(&domain->used_ldb_queues, &queue->domain_list);

	resp->status = 0;
	resp->id = (vdev_req) ? queue->id.virt_id : queue->id.phys_id;

	return 0;
}

static void
dlb2_log_create_dir_queue_args(struct dlb2_hw *hw,
			       u32 domain_id,
			       struct dlb2_create_dir_queue_args *args,
			       bool vdev_req,
			       unsigned int vdev_id)
{
	DLB2_BASE_DBG(hw, "DLB2 create directed queue arguments:\n");
	if (vdev_req)
		DLB2_BASE_DBG(hw, "(Request from vdev %d)\n", vdev_id);
	DLB2_BASE_DBG(hw, "\tDomain ID: %d\n", domain_id);
	DLB2_BASE_DBG(hw, "\tPort ID:   %d\n", args->port_id);
}

/**
 * dlb2_hw_create_dir_queue() - Allocate and initialize a DLB DIR queue.
 * @hw:	  Contains the current state of the DLB2 hardware.
 * @args: User-provided arguments.
 * @resp: Response to user.
 *
 * Return: returns < 0 on error, 0 otherwise. If the driver is unable to
 * satisfy a request, resp->status will be set accordingly.
 */
int dlb2_hw_create_dir_queue(struct dlb2_hw *hw,
			     u32 domain_id,
			     struct dlb2_create_dir_queue_args *args,
			     struct dlb2_cmd_response *resp,
			     bool vdev_req,
			     unsigned int vdev_id)
{
	struct dlb2_dir_pq_pair *queue;
	struct dlb2_domain *domain;
	int ret;

	dlb2_log_create_dir_queue_args(hw, domain_id, args, vdev_req, vdev_id);

	/* Verify that hardware resources are available before attempting to
	 * satisfy the request. This simplifies the error unwinding code.
	 */
	ret = dlb2_verify_create_dir_queue_args(hw,
						domain_id,
						args,
						resp,
						vdev_req,
						vdev_id);
	if (ret)
		return ret;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);
	if (!domain) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: domain not found\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	if (args->port_id != -1)
		queue = dlb2_get_domain_used_dir_pq(args->port_id,
						    vdev_req,
						    domain);
	else
		queue = DLB2_DOM_LIST_HEAD(domain->avail_dir_pq_pairs,
					   typeof(*queue));

	/* Verification should catch this. */
	if (!queue) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: no available dir queues\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	dlb2_configure_dir_queue(hw, domain, queue, args, vdev_req, vdev_id);

	/* Configuration succeeded, so move the resource from the 'avail' to
	 * the 'used' list (if it's not already there).
	 */
	if (args->port_id == -1) {
		dlb2_list_del(&domain->avail_dir_pq_pairs,
			      &queue->domain_list);

		dlb2_list_add(&domain->used_dir_pq_pairs,
			      &queue->domain_list);
	}

	resp->status = 0;

	resp->id = (vdev_req) ? queue->id.virt_id : queue->id.phys_id;

	return 0;
}

static void
dlb2_log_create_ldb_port_args(struct dlb2_hw *hw,
			      u32 domain_id,
			      uintptr_t cq_dma_base,
			      struct dlb2_create_ldb_port_args *args,
			      bool vdev_req,
			      unsigned int vdev_id)
{
	DLB2_BASE_DBG(hw, "DLB2 create load-balanced port arguments:\n");
	if (vdev_req)
		DLB2_BASE_DBG(hw, "(Request from vdev %d)\n", vdev_id);
	DLB2_BASE_DBG(hw, "\tDomain ID:                 %d\n",
		      domain_id);
	DLB2_BASE_DBG(hw, "\tCQ depth:                  %d\n",
		      args->cq_depth);
	DLB2_BASE_DBG(hw, "\tCQ hist list size:         %d\n",
		      args->cq_history_list_size);
	DLB2_BASE_DBG(hw, "\tCQ base address:           0x%lx\n",
		      cq_dma_base);
	DLB2_BASE_DBG(hw, "\tCoS ID:                    %u\n", args->cos_id);
	DLB2_BASE_DBG(hw, "\tStrict CoS allocation:     %u\n",
		      args->cos_strict);
}

/**
 * dlb2_hw_create_ldb_port() - Allocate and initialize a load-balanced port and
 *	its resources.
 * @hw:	  Contains the current state of the DLB2 hardware.
 * @args: User-provided arguments.
 * @resp: Response to user.
 *
 * Return: returns < 0 on error, 0 otherwise. If the driver is unable to
 * satisfy a request, resp->status will be set accordingly.
 */
int dlb2_hw_create_ldb_port(struct dlb2_hw *hw,
			    u32 domain_id,
			    struct dlb2_create_ldb_port_args *args,
			    uintptr_t cq_dma_base,
			    struct dlb2_cmd_response *resp,
			    bool vdev_req,
			    unsigned int vdev_id)
{
	struct dlb2_ldb_port *port;
	struct dlb2_domain *domain;
	int ret, cos_id, i;

	dlb2_log_create_ldb_port_args(hw,
				      domain_id,
				      cq_dma_base,
				      args,
				      vdev_req,
				      vdev_id);

	/* Verify that hardware resources are available before attempting to
	 * satisfy the request. This simplifies the error unwinding code.
	 */
	ret = dlb2_verify_create_ldb_port_args(hw,
					       domain_id,
					       cq_dma_base,
					       args,
					       resp,
					       vdev_req,
					       vdev_id);
	if (ret)
		return ret;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);
	if (!domain) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: domain not found\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	if (args->cos_strict) {
		cos_id = args->cos_id;

		port = DLB2_DOM_LIST_HEAD(domain->avail_ldb_ports[cos_id],
					  typeof(*port));
	} else {
		int idx;

		for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
			idx = (args->cos_id + i) % DLB2_NUM_COS_DOMAINS;

			port = DLB2_DOM_LIST_HEAD(domain->avail_ldb_ports[idx],
						  typeof(*port));
			if (port)
				break;
		}

		cos_id = idx;
	}

	/* Verification should catch this. */
	if (!port) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: no available ldb ports\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	if (port->configured) {
		DLB2_BASE_ERR(hw,
			      "[%s()] Internal error: avail_ldb_ports contains configured ports.\n",
			      __func__);
		return -EFAULT;
	}

	ret = dlb2_configure_ldb_port(hw,
				      domain,
				      port,
				      cq_dma_base,
				      args,
				      vdev_req,
				      vdev_id);
	if (ret < 0)
		return ret;

	/* Configuration succeeded, so move the resource from the 'avail' to
	 * the 'used' list.
	 */
	dlb2_list_del(&domain->avail_ldb_ports[cos_id], &port->domain_list);

	dlb2_list_add(&domain->used_ldb_ports[cos_id], &port->domain_list);

	resp->status = 0;
	resp->id = (vdev_req) ? port->id.virt_id : port->id.phys_id;

	return 0;
}

static void
dlb2_log_create_dir_port_args(struct dlb2_hw *hw,
			      u32 domain_id,
			      uintptr_t cq_dma_base,
			      struct dlb2_create_dir_port_args *args,
			      bool vdev_req,
			      unsigned int vdev_id)
{
	DLB2_BASE_DBG(hw, "DLB2 create directed port arguments:\n");
	if (vdev_req)
		DLB2_BASE_DBG(hw, "(Request from vdev %d)\n", vdev_id);
	DLB2_BASE_DBG(hw, "\tDomain ID:                 %d\n",
		      domain_id);
	DLB2_BASE_DBG(hw, "\tCQ depth:                  %d\n",
		      args->cq_depth);
	DLB2_BASE_DBG(hw, "\tCQ base address:           0x%lx\n",
		      cq_dma_base);
}

/**
 * dlb2_hw_create_dir_port() - Allocate and initialize a DLB directed port
 *	and queue. The port/queue pair have the same ID and name.
 * @hw:	  Contains the current state of the DLB2 hardware.
 * @args: User-provided arguments.
 * @resp: Response to user.
 *
 * Return: returns < 0 on error, 0 otherwise. If the driver is unable to
 * satisfy a request, resp->status will be set accordingly.
 */
int dlb2_hw_create_dir_port(struct dlb2_hw *hw,
			    u32 domain_id,
			    struct dlb2_create_dir_port_args *args,
			    uintptr_t cq_dma_base,
			    struct dlb2_cmd_response *resp,
			    bool vdev_req,
			    unsigned int vdev_id)
{
	struct dlb2_dir_pq_pair *port;
	struct dlb2_domain *domain;
	int ret;

	dlb2_log_create_dir_port_args(hw,
				      domain_id,
				      cq_dma_base,
				      args,
				      vdev_req,
				      vdev_id);

	/* Verify that hardware resources are available before attempting to
	 * satisfy the request. This simplifies the error unwinding code.
	 */
	ret = dlb2_verify_create_dir_port_args(hw,
					       domain_id,
					       cq_dma_base,
					       args,
					       resp,
					       vdev_req,
					       vdev_id);
	if (ret)
		return ret;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);

	if (args->queue_id != -1)
		port = dlb2_get_domain_used_dir_pq(args->queue_id,
						   vdev_req,
						   domain);
	else
		port = DLB2_DOM_LIST_HEAD(domain->avail_dir_pq_pairs,
					  typeof(*port));

	/* Verification should catch this. */
	if (!port) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: no available dir ports\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	dlb2_configure_dir_port(hw,
				domain,
				port,
				cq_dma_base,
				args,
				vdev_req,
				vdev_id);

	/* Configuration succeeded, so move the resource from the 'avail' to
	 * the 'used' list (if it's not already there).
	 */
	if (args->queue_id == -1) {
		dlb2_list_del(&domain->avail_dir_pq_pairs, &port->domain_list);

		dlb2_list_add(&domain->used_dir_pq_pairs, &port->domain_list);
	}

	resp->status = 0;
	resp->id = (vdev_req) ? port->id.virt_id : port->id.phys_id;

	return 0;
}

static void dlb2_log_start_domain(struct dlb2_hw *hw,
				  u32 domain_id,
				  bool vdev_req,
				  unsigned int vdev_id)
{
	DLB2_BASE_DBG(hw, "DLB2 start domain arguments:\n");
	if (vdev_req)
		DLB2_BASE_DBG(hw, "(Request from vdev %d)\n", vdev_id);
	DLB2_BASE_DBG(hw, "\tDomain ID: %d\n", domain_id);
}

/**
 * dlb2_hw_start_domain() - Lock the domain configuration
 * @hw:	  Contains the current state of the DLB2 hardware.
 * @args: User-provided arguments.
 * @resp: Response to user.
 *
 * Return: returns < 0 on error, 0 otherwise. If the driver is unable to
 * satisfy a request, resp->status will be set accordingly.
 */
int
dlb2_hw_start_domain(struct dlb2_hw *hw,
		     u32 domain_id,
		     __attribute((unused)) struct dlb2_start_domain_args *arg,
		     struct dlb2_cmd_response *resp,
		     bool vdev_req,
		     unsigned int vdev_id)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_dir_pq_pair *dir_queue;
	struct dlb2_ldb_queue *ldb_queue;
	struct dlb2_domain *domain;
	int ret;

	dlb2_log_start_domain(hw, domain_id, vdev_req, vdev_id);

	ret = dlb2_verify_start_domain_args(hw,
					    domain_id,
					    resp,
					    vdev_req,
					    vdev_id);
	if (ret)
		return ret;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);
	if (!domain) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: domain not found\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	/* Enable load-balanced and directed queue write permissions for the
	 * queues this domain owns. Without this, the DLB2 will drop all
	 * incoming traffic to those queues.
	 */
	DLB2_DOM_LIST_FOR(domain->used_ldb_queues, ldb_queue, iter) {
		union dlb2_sys_ldb_vasqid_v r0 = { {0} };
		unsigned int offs;

		r0.field.vasqid_v = 1;

		offs = domain->id.phys_id * DLB2_MAX_NUM_LDB_QUEUES +
			ldb_queue->id.phys_id;

		DLB2_CSR_WR(hw, DLB2_SYS_LDB_VASQID_V(offs), r0.val);
	}

	DLB2_DOM_LIST_FOR(domain->used_dir_pq_pairs, dir_queue, iter) {
		union dlb2_sys_dir_vasqid_v r0 = { {0} };
		unsigned int offs;

		r0.field.vasqid_v = 1;

		offs = domain->id.phys_id * DLB2_MAX_NUM_DIR_PORTS +
			dir_queue->id.phys_id;

		DLB2_CSR_WR(hw, DLB2_SYS_DIR_VASQID_V(offs), r0.val);
	}

	dlb2_flush_csr(hw);

	domain->started = true;

	resp->status = 0;

	return 0;
}

static void
dlb2_domain_finish_unmap_port_slot(struct dlb2_hw *hw,
				   struct dlb2_domain *domain,
				   struct dlb2_ldb_port *port,
				   int slot)
{
	enum dlb2_qid_map_state state;
	struct dlb2_ldb_queue *queue;

	queue = &hw->rsrcs.ldb_queues[port->qid_map[slot].qid];

	state = port->qid_map[slot].state;

	/* Update the QID2CQIDX and CQ2QID vectors */
	dlb2_ldb_port_unmap_qid(hw, port, queue);

	/* Ensure the QID will not be serviced by this {CQ, slot} by clearing
	 * the has_work bits
	 */
	dlb2_ldb_port_clear_has_work_bits(hw, port, slot);

	/* Reset the {CQ, slot} to its default state */
	dlb2_ldb_port_set_queue_if_status(hw, port, slot);

	/* Re-enable the CQ if it wasn't manually disabled by the user */
	if (port->enabled)
		dlb2_ldb_port_cq_enable(hw, port);

	/* If there is a mapping that is pending this slot's removal, perform
	 * the mapping now.
	 */
	if (state == DLB2_QUEUE_UNMAP_IN_PROG_PENDING_MAP) {
		struct dlb2_ldb_port_qid_map *map;
		struct dlb2_ldb_queue *map_queue;
		u8 prio;

		map = &port->qid_map[slot];

		map->qid = map->pending_qid;
		map->priority = map->pending_priority;

		map_queue = &hw->rsrcs.ldb_queues[map->qid];
		prio = map->priority;

		dlb2_ldb_port_map_qid(hw, domain, port, map_queue, prio);
	}
}

static bool dlb2_domain_finish_unmap_port(struct dlb2_hw *hw,
					  struct dlb2_domain *domain,
					  struct dlb2_ldb_port *port)
{
	union dlb2_lsp_cq_ldb_infl_cnt r0;
	int i;

	if (port->num_pending_removals == 0)
		return false;

	/* The unmap requires all the CQ's outstanding inflights to be
	 * completed.
	 */
	r0.val = DLB2_CSR_RD(hw, DLB2_LSP_CQ_LDB_INFL_CNT(port->id.phys_id));
	if (r0.field.count > 0)
		return false;

	for (i = 0; i < DLB2_MAX_NUM_QIDS_PER_LDB_CQ; i++) {
		struct dlb2_ldb_port_qid_map *map;

		map = &port->qid_map[i];

		if (map->state != DLB2_QUEUE_UNMAP_IN_PROG &&
		    map->state != DLB2_QUEUE_UNMAP_IN_PROG_PENDING_MAP)
			continue;

		dlb2_domain_finish_unmap_port_slot(hw, domain, port, i);
	}

	return true;
}

static unsigned int
dlb2_domain_finish_unmap_qid_procedures(struct dlb2_hw *hw,
					struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_ldb_port *port;
	int i;

	if (!domain->configured || domain->num_pending_removals == 0)
		return 0;

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i], port, iter)
			dlb2_domain_finish_unmap_port(hw, domain, port);
	}

	return domain->num_pending_removals;
}

unsigned int dlb2_finish_unmap_qid_procedures(struct dlb2_hw *hw)
{
	int i, num = 0;

	/* Finish queue unmap jobs for any domain that needs it */
	for (i = 0; i < DLB2_MAX_NUM_DOMAINS; i++) {
		struct dlb2_domain *domain = &hw->domains[i];

		num += dlb2_domain_finish_unmap_qid_procedures(hw, domain);
	}

	return num;
}

static void dlb2_domain_finish_map_port(struct dlb2_hw *hw,
					struct dlb2_domain *domain,
					struct dlb2_ldb_port *port)
{
	int i;

	for (i = 0; i < DLB2_MAX_NUM_QIDS_PER_LDB_CQ; i++) {
		union dlb2_lsp_qid_ldb_infl_cnt r0;
		struct dlb2_ldb_queue *queue;
		int qid;

		if (port->qid_map[i].state != DLB2_QUEUE_MAP_IN_PROG)
			continue;

		qid = port->qid_map[i].qid;

		queue = dlb2_get_ldb_queue_from_id(hw, qid, false, 0);

		if (!queue) {
			DLB2_BASE_ERR(hw,
				      "[%s()] Internal error: unable to find queue %d\n",
				      __func__, qid);
			continue;
		}

		r0.val = DLB2_CSR_RD(hw, DLB2_LSP_QID_LDB_INFL_CNT(qid));

		if (r0.field.count)
			continue;

		/* Disable the affected CQ, and the CQs already mapped to the
		 * QID, before reading the QID's inflight count a second time.
		 * There is an unlikely race in which the QID may schedule one
		 * more QE after we read an inflight count of 0, and disabling
		 * the CQs guarantees that the race will not occur after a
		 * re-read of the inflight count register.
		 */
		if (port->enabled)
			dlb2_ldb_port_cq_disable(hw, port);

		dlb2_ldb_queue_disable_mapped_cqs(hw, domain, queue);

		r0.val = DLB2_CSR_RD(hw, DLB2_LSP_QID_LDB_INFL_CNT(qid));

		if (r0.field.count) {
			if (port->enabled)
				dlb2_ldb_port_cq_enable(hw, port);

			dlb2_ldb_queue_enable_mapped_cqs(hw, domain, queue);

			continue;
		}

		dlb2_ldb_port_finish_map_qid_dynamic(hw, domain, port, queue);
	}
}

static unsigned int
dlb2_domain_finish_map_qid_procedures(struct dlb2_hw *hw,
				      struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_ldb_port *port;
	int i;

	if (!domain->configured || domain->num_pending_additions == 0)
		return 0;

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i], port, iter)
			dlb2_domain_finish_map_port(hw, domain, port);
	}

	return domain->num_pending_additions;
}

unsigned int dlb2_finish_map_qid_procedures(struct dlb2_hw *hw)
{
	int i, num = 0;

	/* Finish queue map jobs for any domain that needs it */
	for (i = 0; i < DLB2_MAX_NUM_DOMAINS; i++) {
		struct dlb2_domain *domain = &hw->domains[i];

		num += dlb2_domain_finish_map_qid_procedures(hw, domain);
	}

	return num;
}

static void dlb2_log_map_qid(struct dlb2_hw *hw,
			     u32 domain_id,
			     struct dlb2_map_qid_args *args,
			     bool vdev_req,
			     unsigned int vdev_id)
{
	DLB2_BASE_DBG(hw, "DLB2 map QID arguments:\n");
	if (vdev_req)
		DLB2_BASE_DBG(hw, "(Request from vdev %d)\n", vdev_id);
	DLB2_BASE_DBG(hw, "\tDomain ID: %d\n",
		      domain_id);
	DLB2_BASE_DBG(hw, "\tPort ID:   %d\n",
		      args->port_id);
	DLB2_BASE_DBG(hw, "\tQueue ID:  %d\n",
		      args->qid);
	DLB2_BASE_DBG(hw, "\tPriority:  %d\n",
		      args->priority);
}

int dlb2_hw_map_qid(struct dlb2_hw *hw,
		    u32 domain_id,
		    struct dlb2_map_qid_args *args,
		    struct dlb2_cmd_response *resp,
		    bool vdev_req,
		    unsigned int vdev_id)
{
	struct dlb2_ldb_queue *queue;
	enum dlb2_qid_map_state st;
	struct dlb2_ldb_port *port;
	struct dlb2_domain *domain;
	int ret, i, id;
	u8 prio;

	dlb2_log_map_qid(hw, domain_id, args, vdev_req, vdev_id);

	/* Verify that hardware resources are available before attempting to
	 * satisfy the request. This simplifies the error unwinding code.
	 */
	ret = dlb2_verify_map_qid_args(hw,
				       domain_id,
				       args,
				       resp,
				       vdev_req,
				       vdev_id);
	if (ret)
		return ret;

	prio = args->priority;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);
	if (!domain) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: domain not found\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	id = args->port_id;

	port = dlb2_get_domain_used_ldb_port(id, vdev_req, domain);
	if (!port) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: port not found\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	queue = dlb2_get_domain_ldb_queue(args->qid, vdev_req, domain);
	if (!queue) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: queue not found\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	/* If there are any outstanding detach operations for this port,
	 * attempt to complete them. This may be necessary to free up a QID
	 * slot for this requested mapping.
	 */
	if (port->num_pending_removals)
		dlb2_domain_finish_unmap_port(hw, domain, port);

	ret = dlb2_verify_map_qid_slot_available(port, queue, resp);
	if (ret)
		return ret;

	/* Hardware requires disabling the CQ before mapping QIDs. */
	if (port->enabled)
		dlb2_ldb_port_cq_disable(hw, port);

	/* If this is only a priority change, don't perform the full QID->CQ
	 * mapping procedure
	 */
	st = DLB2_QUEUE_MAPPED;
	if (dlb2_port_find_slot_queue(port, st, queue, &i)) {
		if (i >= DLB2_MAX_NUM_QIDS_PER_LDB_CQ) {
			DLB2_BASE_ERR(hw,
				      "[%s():%d] Internal error: port slot tracking failed\n",
				      __func__, __LINE__);
			return -EFAULT;
		}

		if (prio != port->qid_map[i].priority) {
			dlb2_ldb_port_change_qid_priority(hw, port, i, args);
			DLB2_BASE_DBG(hw, "DLB2 map: priority change\n");
		}

		st = DLB2_QUEUE_MAPPED;
		ret = dlb2_port_slot_state_transition(hw, port, queue, i, st);
		if (ret)
			return ret;

		goto map_qid_done;
	}

	st = DLB2_QUEUE_UNMAP_IN_PROG;
	if (dlb2_port_find_slot_queue(port, st, queue, &i)) {
		if (i >= DLB2_MAX_NUM_QIDS_PER_LDB_CQ) {
			DLB2_BASE_ERR(hw,
				      "[%s():%d] Internal error: port slot tracking failed\n",
				      __func__, __LINE__);
			return -EFAULT;
		}

		if (prio != port->qid_map[i].priority) {
			dlb2_ldb_port_change_qid_priority(hw, port, i, args);
			DLB2_BASE_DBG(hw, "DLB2 map: priority change\n");
		}

		st = DLB2_QUEUE_MAPPED;
		ret = dlb2_port_slot_state_transition(hw, port, queue, i, st);
		if (ret)
			return ret;

		goto map_qid_done;
	}

	/* If this is a priority change on an in-progress mapping, don't
	 * perform the full QID->CQ mapping procedure.
	 */
	st = DLB2_QUEUE_MAP_IN_PROG;
	if (dlb2_port_find_slot_queue(port, st, queue, &i)) {
		if (i >= DLB2_MAX_NUM_QIDS_PER_LDB_CQ) {
			DLB2_BASE_ERR(hw,
				      "[%s():%d] Internal error: port slot tracking failed\n",
				      __func__, __LINE__);
			return -EFAULT;
		}

		port->qid_map[i].priority = prio;

		DLB2_BASE_DBG(hw, "DLB2 map: priority change only\n");

		goto map_qid_done;
	}

	/* If this is a priority change on a pending mapping, update the
	 * pending priority
	 */
	if (dlb2_port_find_slot_with_pending_map_queue(port, queue, &i)) {
		if (i >= DLB2_MAX_NUM_QIDS_PER_LDB_CQ) {
			DLB2_BASE_ERR(hw,
				      "[%s():%d] Internal error: port slot tracking failed\n",
				      __func__, __LINE__);
			return -EFAULT;
		}

		port->qid_map[i].pending_priority = prio;

		DLB2_BASE_DBG(hw, "DLB2 map: priority change only\n");

		goto map_qid_done;
	}

	/* If all the CQ's slots are in use, then there's an unmap in progress
	 * (guaranteed by dlb2_verify_map_qid_slot_available()), so add this
	 * mapping to pending_map and return. When the removal is completed for
	 * the slot's current occupant, this mapping will be performed.
	 */
	if (!dlb2_port_find_slot(port, DLB2_QUEUE_UNMAPPED, &i)) {
		if (dlb2_port_find_slot(port, DLB2_QUEUE_UNMAP_IN_PROG, &i)) {
			enum dlb2_qid_map_state st;

			if (i >= DLB2_MAX_NUM_QIDS_PER_LDB_CQ) {
				DLB2_BASE_ERR(hw,
					      "[%s():%d] Internal error: port slot tracking failed\n",
					      __func__, __LINE__);
				return -EFAULT;
			}

			port->qid_map[i].pending_qid = queue->id.phys_id;
			port->qid_map[i].pending_priority = prio;

			st = DLB2_QUEUE_UNMAP_IN_PROG_PENDING_MAP;

			ret = dlb2_port_slot_state_transition(hw, port, queue,
							      i, st);
			if (ret)
				return ret;

			DLB2_BASE_DBG(hw, "DLB2 map: map pending removal\n");

			goto map_qid_done;
		}
	}

	/* If the domain has started, a special "dynamic" CQ->queue mapping
	 * procedure is required in order to safely update the CQ<->QID tables.
	 * The "static" procedure cannot be used when traffic is flowing,
	 * because the CQ<->QID tables cannot be updated atomically and the
	 * scheduler won't see the new mapping unless the queue's if_status
	 * changes, which isn't guaranteed.
	 */
	ret = dlb2_ldb_port_map_qid(hw, domain, port, queue, prio);

	/* If ret is less than zero, it's due to an internal error */
	if (ret < 0)
		return ret;

map_qid_done:
	if (port->enabled)
		dlb2_ldb_port_cq_enable(hw, port);

	resp->status = 0;

	return 0;
}

static void dlb2_log_unmap_qid(struct dlb2_hw *hw,
			       u32 domain_id,
			       struct dlb2_unmap_qid_args *args,
			       bool vdev_req,
			       unsigned int vdev_id)
{
	DLB2_BASE_DBG(hw, "DLB2 unmap QID arguments:\n");
	if (vdev_req)
		DLB2_BASE_DBG(hw, "(Request from vdev %d)\n", vdev_id);
	DLB2_BASE_DBG(hw, "\tDomain ID: %d\n",
		      domain_id);
	DLB2_BASE_DBG(hw, "\tPort ID:   %d\n",
		      args->port_id);
	DLB2_BASE_DBG(hw, "\tQueue ID:  %d\n",
		      args->qid);
	if (args->qid < DLB2_MAX_NUM_LDB_QUEUES)
		DLB2_BASE_DBG(hw, "\tQueue's num mappings:  %d\n",
			      hw->rsrcs.ldb_queues[args->qid].num_mappings);
}

int dlb2_hw_unmap_qid(struct dlb2_hw *hw,
		      u32 domain_id,
		      struct dlb2_unmap_qid_args *args,
		      struct dlb2_cmd_response *resp,
		      bool vdev_req,
		      unsigned int vdev_id)
{
	enum dlb2_qid_map_state st;
	struct dlb2_ldb_queue *queue;
	struct dlb2_ldb_port *port;
	struct dlb2_domain *domain;
	bool unmap_complete;
	int i, ret, id;

	dlb2_log_unmap_qid(hw, domain_id, args, vdev_req, vdev_id);

	/* Verify that hardware resources are available before attempting to
	 * satisfy the request. This simplifies the error unwinding code.
	 */
	ret = dlb2_verify_unmap_qid_args(hw,
					 domain_id,
					 args,
					 resp,
					 vdev_req,
					 vdev_id);
	if (ret)
		return ret;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);
	if (!domain) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: domain not found\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	id = args->port_id;

	port = dlb2_get_domain_used_ldb_port(id, vdev_req, domain);
	if (!port) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: port not found\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	queue = dlb2_get_domain_ldb_queue(args->qid, vdev_req, domain);
	if (!queue) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: queue not found\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	/* If the queue hasn't been mapped yet, we need to update the slot's
	 * state and re-enable the queue's inflights.
	 */
	st = DLB2_QUEUE_MAP_IN_PROG;
	if (dlb2_port_find_slot_queue(port, st, queue, &i)) {
		if (i >= DLB2_MAX_NUM_QIDS_PER_LDB_CQ) {
			DLB2_BASE_ERR(hw,
				      "[%s():%d] Internal error: port slot tracking failed\n",
				      __func__, __LINE__);
			return -EFAULT;
		}

		/* Since the in-progress map was aborted, re-enable the QID's
		 * inflights.
		 */
		if (queue->num_pending_additions == 0)
			dlb2_ldb_queue_set_inflight_limit(hw, queue);

		st = DLB2_QUEUE_UNMAPPED;
		ret = dlb2_port_slot_state_transition(hw, port, queue, i, st);
		if (ret)
			return ret;

		goto unmap_qid_done;
	}

	/* If the queue mapping is on hold pending an unmap, we simply need to
	 * update the slot's state.
	 */
	if (dlb2_port_find_slot_with_pending_map_queue(port, queue, &i)) {
		if (i >= DLB2_MAX_NUM_QIDS_PER_LDB_CQ) {
			DLB2_BASE_ERR(hw,
				      "[%s():%d] Internal error: port slot tracking failed\n",
				      __func__, __LINE__);
			return -EFAULT;
		}

		st = DLB2_QUEUE_UNMAP_IN_PROG;
		ret = dlb2_port_slot_state_transition(hw, port, queue, i, st);
		if (ret)
			return ret;

		goto unmap_qid_done;
	}

	st = DLB2_QUEUE_MAPPED;
	if (!dlb2_port_find_slot_queue(port, st, queue, &i)) {
		DLB2_BASE_ERR(hw,
			      "[%s()] Internal error: no available CQ slots\n",
			      __func__);
		return -EFAULT;
	}

	if (i >= DLB2_MAX_NUM_QIDS_PER_LDB_CQ) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: port slot tracking failed\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	/* QID->CQ mapping removal is an asychronous procedure. It requires
	 * stopping the DLB2 from scheduling this CQ, draining all inflights
	 * from the CQ, then unmapping the queue from the CQ. This function
	 * simply marks the port as needing the queue unmapped, and (if
	 * necessary) starts the unmapping worker thread.
	 */
	dlb2_ldb_port_cq_disable(hw, port);

	st = DLB2_QUEUE_UNMAP_IN_PROG;
	ret = dlb2_port_slot_state_transition(hw, port, queue, i, st);
	if (ret)
		return ret;

	/* Attempt to finish the unmapping now, in case the port has no
	 * outstanding inflights. If that's not the case, this will fail and
	 * the unmapping will be completed at a later time.
	 */
	unmap_complete = dlb2_domain_finish_unmap_port(hw, domain, port);

	/* If the unmapping couldn't complete immediately, launch the worker
	 * thread (if it isn't already launched) to finish it later.
	 */
	if (!unmap_complete && !os_worker_active(hw))
		os_schedule_work(hw);

unmap_qid_done:
	resp->status = 0;

	return 0;
}

static void dlb2_log_enable_port(struct dlb2_hw *hw,
				 u32 domain_id,
				 u32 port_id,
				 bool vdev_req,
				 unsigned int vdev_id)
{
	DLB2_BASE_DBG(hw, "DLB2 enable port arguments:\n");
	if (vdev_req)
		DLB2_BASE_DBG(hw, "(Request from vdev %d)\n", vdev_id);
	DLB2_BASE_DBG(hw, "\tDomain ID: %d\n",
		      domain_id);
	DLB2_BASE_DBG(hw, "\tPort ID:   %d\n",
		      port_id);
}

int dlb2_hw_enable_ldb_port(struct dlb2_hw *hw,
			    u32 domain_id,
			    struct dlb2_enable_ldb_port_args *args,
			    struct dlb2_cmd_response *resp,
			    bool vdev_req,
			    unsigned int vdev_id)
{
	struct dlb2_ldb_port *port;
	struct dlb2_domain *domain;
	int ret, id;

	dlb2_log_enable_port(hw, domain_id, args->port_id, vdev_req, vdev_id);

	/* Verify that hardware resources are available before attempting to
	 * satisfy the request. This simplifies the error unwinding code.
	 */
	ret = dlb2_verify_enable_ldb_port_args(hw,
					       domain_id,
					       args,
					       resp,
					       vdev_req,
					       vdev_id);
	if (ret)
		return ret;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);

	id = args->port_id;

	port = dlb2_get_domain_used_ldb_port(id, vdev_req, domain);

	/* Hardware requires disabling the CQ before unmapping QIDs. */
	if (!port->enabled) {
		dlb2_ldb_port_cq_enable(hw, port);
		port->enabled = true;
	}

	resp->status = 0;

	return 0;
}

static void dlb2_log_disable_port(struct dlb2_hw *hw,
				  u32 domain_id,
				  u32 port_id,
				  bool vdev_req,
				  unsigned int vdev_id)
{
	DLB2_BASE_DBG(hw, "DLB2 disable port arguments:\n");
	if (vdev_req)
		DLB2_BASE_DBG(hw, "(Request from vdev %d)\n", vdev_id);
	DLB2_BASE_DBG(hw, "\tDomain ID: %d\n",
		      domain_id);
	DLB2_BASE_DBG(hw, "\tPort ID:   %d\n",
		      port_id);
}

int dlb2_hw_disable_ldb_port(struct dlb2_hw *hw,
			     u32 domain_id,
			     struct dlb2_disable_ldb_port_args *args,
			     struct dlb2_cmd_response *resp,
			     bool vdev_req,
			     unsigned int vdev_id)
{
	struct dlb2_ldb_port *port;
	struct dlb2_domain *domain;
	int ret, id;

	dlb2_log_disable_port(hw, domain_id, args->port_id, vdev_req, vdev_id);

	/* Verify that hardware resources are available before attempting to
	 * satisfy the request. This simplifies the error unwinding code.
	 */
	ret = dlb2_verify_disable_ldb_port_args(hw,
						domain_id,
						args,
						resp,
						vdev_req,
						vdev_id);
	if (ret)
		return ret;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);
	if (!domain) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: domain not found\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	id = args->port_id;

	port = dlb2_get_domain_used_ldb_port(id, vdev_req, domain);
	if (!port) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: port not found\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	/* Hardware requires disabling the CQ before unmapping QIDs. */
	if (port->enabled) {
		dlb2_ldb_port_cq_disable(hw, port);
		port->enabled = false;
	}

	resp->status = 0;

	return 0;
}

int dlb2_hw_enable_dir_port(struct dlb2_hw *hw,
			    u32 domain_id,
			    struct dlb2_enable_dir_port_args *args,
			    struct dlb2_cmd_response *resp,
			    bool vdev_req,
			    unsigned int vdev_id)
{
	struct dlb2_dir_pq_pair *port;
	struct dlb2_domain *domain;
	int ret, id;

	dlb2_log_enable_port(hw, domain_id, args->port_id, vdev_req, vdev_id);

	/* Verify that hardware resources are available before attempting to
	 * satisfy the request. This simplifies the error unwinding code.
	 */
	ret = dlb2_verify_enable_dir_port_args(hw,
					       domain_id,
					       args,
					       resp,
					       vdev_req,
					       vdev_id);
	if (ret)
		return ret;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);
	if (!domain) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: domain not found\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	id = args->port_id;

	port = dlb2_get_domain_used_dir_pq(id, vdev_req, domain);
	if (!port) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: port not found\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	/* Hardware requires disabling the CQ before unmapping QIDs. */
	if (!port->enabled) {
		dlb2_dir_port_cq_enable(hw, port);
		port->enabled = true;
	}

	resp->status = 0;

	return 0;
}

int dlb2_hw_disable_dir_port(struct dlb2_hw *hw,
			     u32 domain_id,
			     struct dlb2_disable_dir_port_args *args,
			     struct dlb2_cmd_response *resp,
			     bool vdev_req,
			     unsigned int vdev_id)
{
	struct dlb2_dir_pq_pair *port;
	struct dlb2_domain *domain;
	int ret, id;

	dlb2_log_disable_port(hw, domain_id, args->port_id, vdev_req, vdev_id);

	/* Verify that hardware resources are available before attempting to
	 * satisfy the request. This simplifies the error unwinding code.
	 */
	ret = dlb2_verify_disable_dir_port_args(hw,
						domain_id,
						args,
						resp,
						vdev_req,
						vdev_id);
	if (ret)
		return ret;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);
	if (!domain) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: domain not found\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	id = args->port_id;

	port = dlb2_get_domain_used_dir_pq(id, vdev_req, domain);
	if (!port) {
		DLB2_BASE_ERR(hw,
			      "[%s():%d] Internal error: port not found\n",
			      __func__, __LINE__);
		return -EFAULT;
	}

	/* Hardware requires disabling the CQ before unmapping QIDs. */
	if (port->enabled) {
		dlb2_dir_port_cq_disable(hw, port);
		port->enabled = false;
	}

	resp->status = 0;

	return 0;
}

int dlb2_notify_vf(struct dlb2_hw *hw,
		   unsigned int vf_id,
		   enum dlb2_mbox_vf_notification_type notification)
{
	struct dlb2_mbox_vf_notification_cmd_req req;
	int ret, retry_cnt;

	req.hdr.type = DLB2_MBOX_VF_CMD_NOTIFICATION;
	req.notification = notification;

	ret = dlb2_pf_write_vf_mbox_req(hw, vf_id, &req, sizeof(req));
	if (ret)
		return ret;

	dlb2_send_async_pf_to_vdev_msg(hw, vf_id);

	/* Timeout after 1 second of inactivity */
	retry_cnt = 1000;
	do {
		if (dlb2_pf_to_vdev_complete(hw, vf_id))
			break;
		os_msleep(1);
	} while (--retry_cnt);

	if (!retry_cnt) {
		DLB2_BASE_ERR(hw,
			      "PF driver timed out waiting for mbox response\n");
		return -ETIMEDOUT;
	}

	/* No response data expected for notifications. */

	return 0;
}

int dlb2_vdev_in_use(struct dlb2_hw *hw, unsigned int id)
{
	struct dlb2_mbox_vf_in_use_cmd_resp resp;
	struct dlb2_mbox_vf_in_use_cmd_req req;
	int ret, retry_cnt;

	req.hdr.type = DLB2_MBOX_VF_CMD_IN_USE;

	ret = dlb2_pf_write_vf_mbox_req(hw, id, &req, sizeof(req));
	if (ret)
		return ret;

	dlb2_send_async_pf_to_vdev_msg(hw, id);

	/* Timeout after 1 second of inactivity */
	retry_cnt = 1000;
	do {
		if (dlb2_pf_to_vdev_complete(hw, id))
			break;
		os_msleep(1);
	} while (--retry_cnt);

	if (!retry_cnt) {
		DLB2_BASE_ERR(hw,
			      "PF driver timed out waiting for mbox response\n");
		return -ETIMEDOUT;
	}

	ret = dlb2_pf_read_vf_mbox_resp(hw, id, &resp, sizeof(resp));
	if (ret)
		return ret;

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		DLB2_BASE_ERR(hw,
			      "[%s()]: failed with mailbox error: %s\n",
			      __func__,
			      DLB2_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.in_use;
}

static int dlb2_notify_vf_alarm(struct dlb2_hw *hw,
				unsigned int vf_id,
				u32 domain_id,
				u32 alert_id,
				u32 aux_alert_data)
{
	struct dlb2_mbox_vf_alert_cmd_req req;
	int ret, retry_cnt;

	req.hdr.type = DLB2_MBOX_VF_CMD_DOMAIN_ALERT;
	req.domain_id = domain_id;
	req.alert_id = alert_id;
	req.aux_alert_data = aux_alert_data;

	ret = dlb2_pf_write_vf_mbox_req(hw, vf_id, &req, sizeof(req));
	if (ret)
		return ret;

	dlb2_send_async_pf_to_vdev_msg(hw, vf_id);

	/* Timeout after 1 second of inactivity */
	retry_cnt = 1000;
	do {
		if (dlb2_pf_to_vdev_complete(hw, vf_id))
			break;
		os_msleep(1);
	} while (--retry_cnt);

	if (!retry_cnt) {
		DLB2_BASE_ERR(hw,
			      "PF driver timed out waiting for mbox response\n");
		return -ETIMEDOUT;
	}

	/* No response data expected for alarm notifications. */

	return 0;
}

void dlb2_set_msix_mode(struct dlb2_hw *hw, int mode)
{
	union dlb2_sys_msix_mode r0 = { {0} };

	r0.field.mode = mode;

	DLB2_CSR_WR(hw, DLB2_SYS_MSIX_MODE, r0.val);
}

int dlb2_configure_ldb_cq_interrupt(struct dlb2_hw *hw,
				    int port_id,
				    int vector,
				    int mode,
				    unsigned int vf,
				    unsigned int owner_vf,
				    u16 threshold)
{
	union dlb2_chp_ldb_cq_int_depth_thrsh r0 = { {0} };
	union dlb2_chp_ldb_cq_int_enb r1 = { {0} };
	union dlb2_sys_ldb_cq_isr r2 = { {0} };
	struct dlb2_ldb_port *port;
	bool vdev_req;

	vdev_req = (mode == DLB2_CQ_ISR_MODE_MSI ||
		    mode == DLB2_CQ_ISR_MODE_ADI);

	port = dlb2_get_ldb_port_from_id(hw, port_id, vdev_req, vf);
	if (!port) {
		DLB2_BASE_ERR(hw,
			      "[%s()]: Internal error: failed to enable LDB CQ int\n\tport_id: %u, vdev_req: %u, vdev: %u\n",
			      __func__, port_id, vdev_req, vf);
		return -EINVAL;
	}

	/* Trigger the interrupt when threshold or more QEs arrive in the CQ */
	r0.field.depth_threshold = threshold - 1;

	DLB2_CSR_WR(hw,
		    DLB2_CHP_LDB_CQ_INT_DEPTH_THRSH(port->id.phys_id),
		    r0.val);

	r1.field.en_depth = 1;

	DLB2_CSR_WR(hw, DLB2_CHP_LDB_CQ_INT_ENB(port->id.phys_id), r1.val);

	r2.field.vector = vector;
	r2.field.vf = owner_vf;
	r2.field.en_code = mode;

	DLB2_CSR_WR(hw, DLB2_SYS_LDB_CQ_ISR(port->id.phys_id), r2.val);

	return 0;
}

int dlb2_configure_dir_cq_interrupt(struct dlb2_hw *hw,
				    int port_id,
				    int vector,
				    int mode,
				    unsigned int vf,
				    unsigned int owner_vf,
				    u16 threshold)
{
	union dlb2_chp_dir_cq_int_depth_thrsh r0 = { {0} };
	union dlb2_chp_dir_cq_int_enb r1 = { {0} };
	union dlb2_sys_dir_cq_isr r2 = { {0} };
	struct dlb2_dir_pq_pair *port;
	bool vdev_req;

	vdev_req = (mode == DLB2_CQ_ISR_MODE_MSI ||
		    mode == DLB2_CQ_ISR_MODE_ADI);

	port = dlb2_get_dir_pq_from_id(hw, port_id, vdev_req, vf);
	if (!port) {
		DLB2_BASE_ERR(hw,
			      "[%s()]: Internal error: failed to enable DIR CQ int\n\tport_id: %u, vdev_req: %u, vdev: %u\n",
			      __func__, port_id, vdev_req, vf);
		return -EINVAL;
	}

	/* Trigger the interrupt when threshold or more QEs arrive in the CQ */
	r0.field.depth_threshold = threshold - 1;

	DLB2_CSR_WR(hw,
		    DLB2_CHP_DIR_CQ_INT_DEPTH_THRSH(port->id.phys_id),
		    r0.val);

	r1.field.en_depth = 1;

	DLB2_CSR_WR(hw, DLB2_CHP_DIR_CQ_INT_ENB(port->id.phys_id), r1.val);

	r2.field.vector = vector;
	r2.field.vf = owner_vf;
	r2.field.en_code = mode;

	DLB2_CSR_WR(hw, DLB2_SYS_DIR_CQ_ISR(port->id.phys_id), r2.val);

	return 0;
}

int dlb2_cq_depth(struct dlb2_hw *hw,
		  int port_id,
		  bool is_ldb,
		  bool vdev_req,
		  unsigned int vdev_id)
{
	union dlb2_chp_ldb_cq_depth r0;
	u32 reg;

	if (vdev_req && is_ldb) {
		struct dlb2_ldb_port *ldb_port;

		ldb_port = dlb2_get_ldb_port_from_id(hw, port_id,
						     true, vdev_id);

		if (!ldb_port || !ldb_port->configured)
			return -EINVAL;

		port_id = ldb_port->id.phys_id;
	} else if (vdev_req && !is_ldb) {
		struct dlb2_dir_pq_pair *dir_port;

		dir_port = dlb2_get_dir_pq_from_id(hw, port_id, true, vdev_id);

		if (!dir_port || !dir_port->port_configured)
			return -EINVAL;

		port_id = dir_port->id.phys_id;
	}

	if (is_ldb)
		reg = DLB2_CHP_LDB_CQ_DEPTH(port_id);
	else
		reg = DLB2_CHP_DIR_CQ_DEPTH(port_id);

	r0.val = DLB2_CSR_RD(hw, reg);

	return r0.field.depth;
}

int dlb2_arm_cq_interrupt(struct dlb2_hw *hw,
			  int port_id,
			  bool is_ldb,
			  bool vdev_req,
			  unsigned int vdev_id)
{
	u32 val;
	u32 reg;

	if (vdev_req && is_ldb) {
		struct dlb2_ldb_port *ldb_port;

		ldb_port = dlb2_get_ldb_port_from_id(hw, port_id,
						     true, vdev_id);

		if (!ldb_port || !ldb_port->configured)
			return -EINVAL;

		port_id = ldb_port->id.phys_id;
	} else if (vdev_req && !is_ldb) {
		struct dlb2_dir_pq_pair *dir_port;

		dir_port = dlb2_get_dir_pq_from_id(hw, port_id, true, vdev_id);

		if (!dir_port || !dir_port->port_configured)
			return -EINVAL;

		port_id = dir_port->id.phys_id;
	}

	val = 1 << (port_id % 32);

	if (is_ldb && port_id < 32)
		reg = DLB2_CHP_LDB_CQ_INTR_ARMED0;
	else if (is_ldb && port_id < 64)
		reg = DLB2_CHP_LDB_CQ_INTR_ARMED1;
	else if (!is_ldb && port_id < 32)
		reg = DLB2_CHP_DIR_CQ_INTR_ARMED0;
	else
		reg = DLB2_CHP_DIR_CQ_INTR_ARMED1;

	DLB2_CSR_WR(hw, reg, val);

	dlb2_flush_csr(hw);

	return 0;
}

void dlb2_read_compressed_cq_intr_status(struct dlb2_hw *hw,
					 u32 *ldb_interrupts,
					 u32 *dir_interrupts)
{
	/* Read every CQ's interrupt status */

	ldb_interrupts[0] = DLB2_CSR_RD(hw,
					DLB2_SYS_LDB_CQ_31_0_OCC_INT_STS);
	ldb_interrupts[1] = DLB2_CSR_RD(hw,
					DLB2_SYS_LDB_CQ_63_32_OCC_INT_STS);

	dir_interrupts[0] = DLB2_CSR_RD(hw,
					DLB2_SYS_DIR_CQ_31_0_OCC_INT_STS);
	dir_interrupts[1] = DLB2_CSR_RD(hw,
					DLB2_SYS_DIR_CQ_63_32_OCC_INT_STS);
}

void dlb2_ack_msix_interrupt(struct dlb2_hw *hw, int vector)
{
	union dlb2_sys_msix_ack r0 = { {0} };

	switch (vector) {
	case 0:
		r0.field.msix_0_ack = 1;
		break;
	case 1:
		r0.field.msix_1_ack = 1;
		/*
		 * CSSY-1650
		 * workaround h/w bug for lost MSI-X interrupts
		 *
		 * The recommended workaround for acknowledging
		 * vector 1 interrupts is :
		 *   1: set   MSI-X mask
		 *   2: set   MSIX_PASSTHROUGH
		 *   3: clear MSIX_ACK
		 *   4: clear MSIX_PASSTHROUGH
		 *   5: clear MSI-X mask
		 *
		 * The MSIX-ACK (step 3) is cleared for all vectors
		 * below. We handle steps 1 & 2 for vector 1 here.
		 *
		 * The bitfields for MSIX_ACK and MSIX_PASSTHRU are
		 * defined the same, so we just use the MSIX_ACK
		 * value when writing to PASSTHRU.
		 */

		/* set MSI-X mask and passthrough for vector 1 */
		DLB2_FUNC_WR(hw, DLB2_MSIX_MEM_VECTOR_CTRL(1), 1);
		DLB2_CSR_WR(hw, DLB2_SYS_MSIX_PASSTHRU, r0.val);
		break;
	}

	/* clear MSIX_ACK (write one to clear) */
	DLB2_CSR_WR(hw, DLB2_SYS_MSIX_ACK, r0.val);

	if (vector == 1) {
		/*
		 * finish up steps 4 & 5 of the workaround -
		 * clear pasthrough and mask
		 */
		DLB2_CSR_WR(hw, DLB2_SYS_MSIX_PASSTHRU, 0);
		DLB2_FUNC_WR(hw, DLB2_MSIX_MEM_VECTOR_CTRL(1), 0);
	}

	dlb2_flush_csr(hw);
}

void dlb2_ack_compressed_cq_intr(struct dlb2_hw *hw,
				 u32 *ldb_interrupts,
				 u32 *dir_interrupts)
{
	/* Write back the status regs to ack the interrupts */
	if (ldb_interrupts[0])
		DLB2_CSR_WR(hw,
			    DLB2_SYS_LDB_CQ_31_0_OCC_INT_STS,
			    ldb_interrupts[0]);
	if (ldb_interrupts[1])
		DLB2_CSR_WR(hw,
			    DLB2_SYS_LDB_CQ_63_32_OCC_INT_STS,
			    ldb_interrupts[1]);

	if (dir_interrupts[0])
		DLB2_CSR_WR(hw,
			    DLB2_SYS_DIR_CQ_31_0_OCC_INT_STS,
			    dir_interrupts[0]);
	if (dir_interrupts[1])
		DLB2_CSR_WR(hw,
			    DLB2_SYS_DIR_CQ_63_32_OCC_INT_STS,
			    dir_interrupts[1]);

	dlb2_ack_msix_interrupt(hw, DLB2_PF_COMPRESSED_MODE_CQ_VECTOR_ID);
}

u32 dlb2_read_vf_intr_status(struct dlb2_hw *hw)
{
	return DLB2_FUNC_RD(hw, DLB2_FUNC_VF_VF_MSI_ISR);
}

void dlb2_ack_vf_intr_status(struct dlb2_hw *hw, u32 interrupts)
{
	DLB2_FUNC_WR(hw, DLB2_FUNC_VF_VF_MSI_ISR, interrupts);
}

void dlb2_ack_vf_msi_intr(struct dlb2_hw *hw, u32 interrupts)
{
	DLB2_FUNC_WR(hw, DLB2_FUNC_VF_VF_MSI_ISR_PEND, interrupts);
}

void dlb2_ack_pf_mbox_int(struct dlb2_hw *hw)
{
	union dlb2_func_vf_pf2vf_mailbox_isr r0;

	if (hw->virt_mode == DLB2_VIRT_SIOV)
		r0.field.pf_isr = 0;
	else
		r0.field.pf_isr = 1;

	DLB2_FUNC_WR(hw, DLB2_FUNC_VF_PF2VF_MAILBOX_ISR, r0.val);
}

void dlb2_enable_ingress_error_alarms(struct dlb2_hw *hw)
{
	union dlb2_sys_ingress_alarm_enbl r0;

	r0.val = DLB2_CSR_RD(hw, DLB2_SYS_INGRESS_ALARM_ENBL);

	r0.field.illegal_hcw = 1;
	r0.field.illegal_pp = 1;
	r0.field.illegal_pasid = 1;
	r0.field.illegal_qid = 1;
	r0.field.disabled_qid = 1;
	r0.field.illegal_ldb_qid_cfg = 1;

	DLB2_CSR_WR(hw, DLB2_SYS_INGRESS_ALARM_ENBL, r0.val);
}

void dlb2_disable_ingress_error_alarms(struct dlb2_hw *hw)
{
	union dlb2_sys_ingress_alarm_enbl r0;

	r0.val = DLB2_CSR_RD(hw, DLB2_SYS_INGRESS_ALARM_ENBL);

	r0.field.illegal_hcw = 0;
	r0.field.illegal_pp = 0;
	r0.field.illegal_pasid = 0;
	r0.field.illegal_qid = 0;
	r0.field.disabled_qid = 0;
	r0.field.illegal_ldb_qid_cfg = 0;

	DLB2_CSR_WR(hw, DLB2_SYS_INGRESS_ALARM_ENBL, r0.val);
}

static void dlb2_log_alarm_syndrome(struct dlb2_hw *hw,
				    const char *str,
				    union dlb2_sys_alarm_hw_synd r0)
{
	DLB2_BASE_ERR(hw, "%s:\n", str);
	DLB2_BASE_ERR(hw, "\tsyndrome: 0x%x\n", r0.field.syndrome);
	DLB2_BASE_ERR(hw, "\trtype:    0x%x\n", r0.field.rtype);
	DLB2_BASE_ERR(hw, "\talarm:    0x%x\n", r0.field.alarm);
	DLB2_BASE_ERR(hw, "\tcwd:      0x%x\n", r0.field.cwd);
	DLB2_BASE_ERR(hw, "\tvf_pf_mb: 0x%x\n", r0.field.vf_pf_mb);
	DLB2_BASE_ERR(hw, "\tcls:      0x%x\n", r0.field.cls);
	DLB2_BASE_ERR(hw, "\taid:      0x%x\n", r0.field.aid);
	DLB2_BASE_ERR(hw, "\tunit:     0x%x\n", r0.field.unit);
	DLB2_BASE_ERR(hw, "\tsource:   0x%x\n", r0.field.source);
	DLB2_BASE_ERR(hw, "\tmore:     0x%x\n", r0.field.more);
	DLB2_BASE_ERR(hw, "\tvalid:    0x%x\n", r0.field.valid);
}

static void dlb2_log_pf_vf_syndrome(struct dlb2_hw *hw,
				    const char *str,
				    union dlb2_sys_alarm_pf_synd0 r0,
				    union dlb2_sys_alarm_pf_synd1 r1,
				    union dlb2_sys_alarm_pf_synd2 r2)
{
	DLB2_BASE_ERR(hw, "%s:\n", str);
	DLB2_BASE_ERR(hw, "\tsyndrome:     0x%x\n", r0.field.syndrome);
	DLB2_BASE_ERR(hw, "\trtype:        0x%x\n", r0.field.rtype);
	DLB2_BASE_ERR(hw, "\tis_ldb:       0x%x\n", r0.field.is_ldb);
	DLB2_BASE_ERR(hw, "\tcls:          0x%x\n", r0.field.cls);
	DLB2_BASE_ERR(hw, "\taid:          0x%x\n", r0.field.aid);
	DLB2_BASE_ERR(hw, "\tunit:         0x%x\n", r0.field.unit);
	DLB2_BASE_ERR(hw, "\tsource:       0x%x\n", r0.field.source);
	DLB2_BASE_ERR(hw, "\tmore:         0x%x\n", r0.field.more);
	DLB2_BASE_ERR(hw, "\tvalid:        0x%x\n", r0.field.valid);
	DLB2_BASE_ERR(hw, "\tdsi:          0x%x\n", r1.field.dsi);
	DLB2_BASE_ERR(hw, "\tqid:          0x%x\n", r1.field.qid);
	DLB2_BASE_ERR(hw, "\tqtype:        0x%x\n", r1.field.qtype);
	DLB2_BASE_ERR(hw, "\tqpri:         0x%x\n", r1.field.qpri);
	DLB2_BASE_ERR(hw, "\tmsg_type:     0x%x\n", r1.field.msg_type);
	DLB2_BASE_ERR(hw, "\tlock_id:      0x%x\n", r2.field.lock_id);
	DLB2_BASE_ERR(hw, "\tmeas:         0x%x\n", r2.field.meas);
	DLB2_BASE_ERR(hw, "\tdebug:        0x%x\n", r2.field.debug);
	DLB2_BASE_ERR(hw, "\tcq_pop:       0x%x\n", r2.field.cq_pop);
	DLB2_BASE_ERR(hw, "\tqe_uhl:       0x%x\n", r2.field.qe_uhl);
	DLB2_BASE_ERR(hw, "\tqe_orsp:      0x%x\n", r2.field.qe_orsp);
	DLB2_BASE_ERR(hw, "\tqe_valid:     0x%x\n", r2.field.qe_valid);
	DLB2_BASE_ERR(hw, "\tcq_int_rearm: 0x%x\n", r2.field.cq_int_rearm);
	DLB2_BASE_ERR(hw, "\tdsi_error:    0x%x\n", r2.field.dsi_error);
}

static void dlb2_clear_syndrome_register(struct dlb2_hw *hw, u32 offset)
{
	union dlb2_sys_alarm_hw_synd r0 = { {0} };

	r0.field.valid = 1;
	r0.field.more = 1;

	DLB2_CSR_WR(hw, offset, r0.val);
}

void dlb2_process_alarm_interrupt(struct dlb2_hw *hw)
{
	union dlb2_sys_alarm_hw_synd r0;

	DLB2_BASE_DBG(hw, "Processing alarm interrupt\n");

	r0.val = DLB2_CSR_RD(hw, DLB2_SYS_ALARM_HW_SYND);

	dlb2_log_alarm_syndrome(hw, "HW alarm syndrome", r0);

	dlb2_clear_syndrome_register(hw, DLB2_SYS_ALARM_HW_SYND);
}

static u32 dlb2_hw_read_vf_to_pf_int_bitvec(struct dlb2_hw *hw)
{
	/* The PF has one VF->PF MBOX ISR register per VF space, but they all
	 * alias to the same physical register.
	 */
	return DLB2_FUNC_RD(hw, DLB2_FUNC_PF_VF2PF_MAILBOX_ISR(0));
}

static u32 dlb2_sw_read_vdev_to_pf_int_bitvec(struct dlb2_hw *hw)
{
	u32 bitvec = 0;
	int i;

	for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++) {
		if (*hw->mbox[i].vdev_to_pf.isr_in_progress)
			bitvec |= (1 << i);
	}

	return bitvec;
}

u32 dlb2_read_vdev_to_pf_int_bitvec(struct dlb2_hw *hw)
{
	if (hw->virt_mode == DLB2_VIRT_SIOV)
		return dlb2_sw_read_vdev_to_pf_int_bitvec(hw);
	else
		return dlb2_hw_read_vf_to_pf_int_bitvec(hw);
}

static void dlb2_hw_ack_vf_mbox_int(struct dlb2_hw *hw, u32 bitvec)
{
	/* The PF has one VF->PF MBOX ISR register per VF space, but
	 * they all alias to the same physical register.
	 */
	DLB2_FUNC_WR(hw, DLB2_FUNC_PF_VF2PF_MAILBOX_ISR(0), bitvec);
}

static void dlb2_sw_ack_vdev_mbox_int(struct dlb2_hw *hw, u32 bitvec)
{
	int i;

	for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++) {
		if ((bitvec & (1 << i)) == 0)
			continue;

		*hw->mbox[i].vdev_to_pf.isr_in_progress = 0;
	}
}

void dlb2_ack_vdev_mbox_int(struct dlb2_hw *hw, u32 bitvec)
{
	if (hw->virt_mode == DLB2_VIRT_SIOV)
		dlb2_sw_ack_vdev_mbox_int(hw, bitvec);
	else
		dlb2_hw_ack_vf_mbox_int(hw, bitvec);
}

u32 dlb2_read_vf_flr_int_bitvec(struct dlb2_hw *hw)
{
	/* The PF has one VF->PF FLR ISR register per VF space, but they all
	 * alias to the same physical register.
	 */
	return DLB2_FUNC_RD(hw, DLB2_FUNC_PF_VF2PF_FLR_ISR(0));
}

void dlb2_ack_vf_flr_int(struct dlb2_hw *hw, u32 bitvec)
{
	union dlb2_iosf_func_vf_bar_dsbl r0 = { {0} };
	int i;

	if (!bitvec)
		return;

	/* Re-enable access to the VF BAR */
	r0.field.func_vf_bar_dis = 0;
	for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++) {
		if (!(bitvec & (1 << i)))
			continue;

		DLB2_CSR_WR(hw, DLB2_IOSF_FUNC_VF_BAR_DSBL(i), r0.val);
	}

	/* Notify the VF driver that the reset has completed */
	DLB2_FUNC_WR(hw, DLB2_FUNC_PF_VF_RESET_IN_PROGRESS(0), bitvec);

	/* Mark the FLR ISR as complete */
	DLB2_FUNC_WR(hw, DLB2_FUNC_PF_VF2PF_FLR_ISR(0), bitvec);
}

void dlb2_ack_vdev_to_pf_int(struct dlb2_hw *hw,
			     u32 mbox_bitvec,
			     u32 flr_bitvec)
{
	int i;

	/* If using S-IOV, this is a noop */
	if (hw->virt_mode == DLB2_VIRT_SIOV)
		return;

	for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++) {
		union dlb2_func_pf_vf2pf_isr_pend r0 = { {0} };

		if (!((mbox_bitvec & (1 << i)) || (flr_bitvec & (1 << i))))
			continue;

		/* Unset the VF's ISR pending bit */
		r0.field.isr_pend = 1;
		DLB2_FUNC_WR(hw, DLB2_FUNC_PF_VF2PF_ISR_PEND(i), r0.val);
	}
}

void dlb2_process_wdt_interrupt(struct dlb2_hw *hw)
{
	u32 alert_id = DLB2_DOMAIN_ALERT_CQ_WATCHDOG_TIMEOUT;
	union dlb2_chp_cfg_dir_wdto_0 r0;
	union dlb2_chp_cfg_dir_wdto_1 r1;
	union dlb2_chp_cfg_ldb_wdto_0 r2;
	union dlb2_chp_cfg_ldb_wdto_1 r3;
	int i;

	r0.val = DLB2_CSR_RD(hw, DLB2_CHP_CFG_DIR_WDTO_0);
	r1.val = DLB2_CSR_RD(hw, DLB2_CHP_CFG_DIR_WDTO_1);
	r2.val = DLB2_CSR_RD(hw, DLB2_CHP_CFG_LDB_WDTO_0);
	r3.val = DLB2_CSR_RD(hw, DLB2_CHP_CFG_LDB_WDTO_1);

	if (r0.val == 0xFFFFFFFF &&
	    r1.val == 0xFFFFFFFF &&
	    r2.val == 0xFFFFFFFF &&
	    r3.val == 0xFFFFFFFF)
		return;

	/* Alert applications for affected directed ports */
	for (i = 0; i < DLB2_MAX_NUM_DIR_PORTS; i++) {
		struct dlb2_dir_pq_pair *port;
		int idx = i % 32;

		if (i < 32 && !(r0.val & (1 << idx)))
			continue;
		if (i >= 32 && !(r1.val & (1 << idx)))
			continue;

		port = dlb2_get_dir_pq_from_id(hw, i, false, 0);
		if (!port) {
			DLB2_BASE_ERR(hw,
				      "[%s()]: Internal error: unable to find DIR port %u\n",
				      __func__, i);
			return;
		}

		if (port->id.vdev_owned)
			dlb2_notify_vf_alarm(hw,
					     port->id.vdev_id,
					     port->domain_id.virt_id,
					     alert_id,
					     port->id.virt_id);
		else
			os_notify_user_space(hw,
					     port->domain_id.phys_id,
					     alert_id,
					     i);
	}

	/* Alert applications for affected load-balanced ports */
	for (i = 0; i < DLB2_MAX_NUM_LDB_PORTS; i++) {
		struct dlb2_ldb_port *port;
		int idx = i % 32;

		if (i < 32 && !(r2.val & (1 << idx)))
			continue;
		if (i >= 32 && !(r3.val & (1 << idx)))
			continue;

		port = dlb2_get_ldb_port_from_id(hw, i, false, 0);
		if (!port) {
			DLB2_BASE_ERR(hw,
				      "[%s()]: Internal error: unable to find LDB port %u\n",
				      __func__, i);
			return;
		}

		/* aux_alert_data[8] is 1 to indicate a load-balanced port */
		if (port->id.vdev_owned)
			dlb2_notify_vf_alarm(hw,
					     port->id.vdev_id,
					     port->domain_id.virt_id,
					     alert_id,
					     (1 << 8) | port->id.virt_id);
		else
			os_notify_user_space(hw,
					     port->domain_id.phys_id,
					     alert_id,
					     (1 << 8) | i);
	}

	/* Clear watchdog timeout flag(s) (W1CLR) */
	DLB2_CSR_WR(hw, DLB2_CHP_CFG_DIR_WDTO_0, r0.val);
	DLB2_CSR_WR(hw, DLB2_CHP_CFG_DIR_WDTO_1, r1.val);
	DLB2_CSR_WR(hw, DLB2_CHP_CFG_LDB_WDTO_0, r2.val);
	DLB2_CSR_WR(hw, DLB2_CHP_CFG_LDB_WDTO_1, r3.val);

	dlb2_flush_csr(hw);

	/* Re-enable watchdog timeout(s) (W1CLR) */
	DLB2_CSR_WR(hw, DLB2_CHP_CFG_DIR_WD_DISABLE0, r0.val);
	DLB2_CSR_WR(hw, DLB2_CHP_CFG_DIR_WD_DISABLE1, r1.val);
	DLB2_CSR_WR(hw, DLB2_CHP_CFG_LDB_WD_DISABLE0, r2.val);
	DLB2_CSR_WR(hw, DLB2_CHP_CFG_LDB_WD_DISABLE1, r3.val);
}

static void dlb2_process_ingress_error(struct dlb2_hw *hw,
				       union dlb2_sys_alarm_pf_synd0 r0,
				       bool vf_error,
				       unsigned int vf_id)
{
	struct dlb2_domain *domain;
	u32 alert_id;
	u8 port_id;
	bool is_ldb;

	port_id = r0.field.syndrome & 0x7F;
	if (r0.field.source == DLB2_ALARM_HW_SOURCE_SYS)
		is_ldb = r0.field.is_ldb;
	else
		is_ldb = (r0.field.syndrome & 0x80) != 0;

	/* Get the domain ID and, if it's a VF domain, the virtual port ID */
	if (is_ldb) {
		struct dlb2_ldb_port *port;

		port = dlb2_get_ldb_port_from_id(hw, port_id, vf_error, vf_id);
		if (!port) {
			DLB2_BASE_ERR(hw,
				      "[%s()]: Internal error: unable to find LDB port\n\tport: %u, vf_error: %u, vf_id: %u\n",
				      __func__, port_id, vf_error, vf_id);
			return;
		}

		domain = &hw->domains[port->domain_id.phys_id];
	} else {
		struct dlb2_dir_pq_pair *port;

		port = dlb2_get_dir_pq_from_id(hw, port_id, vf_error, vf_id);
		if (!port) {
			DLB2_BASE_ERR(hw,
				      "[%s()]: Internal error: unable to find DIR port\n\tport: %u, vf_error: %u, vf_id: %u\n",
				      __func__, port_id, vf_error, vf_id);
			return;
		}

		domain = &hw->domains[port->domain_id.phys_id];
	}

	if (r0.field.unit == DLB2_ALARM_HW_UNIT_CHP &&
	    r0.field.aid == DLB2_ALARM_HW_CHP_AID_ILLEGAL_ENQ)
		alert_id = DLB2_DOMAIN_ALERT_PP_ILLEGAL_ENQ;
	else if (r0.field.unit == DLB2_ALARM_HW_UNIT_CHP &&
		 r0.field.aid == DLB2_ALARM_HW_CHP_AID_EXCESS_TOKEN_POPS)
		alert_id = DLB2_DOMAIN_ALERT_PP_EXCESS_TOKEN_POPS;
	else if (r0.field.source == DLB2_ALARM_HW_SOURCE_SYS &&
		 r0.field.aid == DLB2_ALARM_SYS_AID_ILLEGAL_HCW)
		alert_id = DLB2_DOMAIN_ALERT_ILLEGAL_HCW;
	else if (r0.field.source == DLB2_ALARM_HW_SOURCE_SYS &&
		 r0.field.aid == DLB2_ALARM_SYS_AID_ILLEGAL_QID)
		alert_id = DLB2_DOMAIN_ALERT_ILLEGAL_QID;
	else if (r0.field.source == DLB2_ALARM_HW_SOURCE_SYS &&
		 r0.field.aid == DLB2_ALARM_SYS_AID_DISABLED_QID)
		alert_id = DLB2_DOMAIN_ALERT_DISABLED_QID;
	else
		return;

	if (vf_error)
		dlb2_notify_vf_alarm(hw,
				     vf_id,
				     domain->id.virt_id,
				     alert_id,
				     (is_ldb << 8) | port_id);
	else
		os_notify_user_space(hw,
				     domain->id.phys_id,
				     alert_id,
				     (is_ldb << 8) | port_id);
}

bool dlb2_process_ingress_error_interrupt(struct dlb2_hw *hw)
{
	union dlb2_sys_alarm_pf_synd0 r0;
	union dlb2_sys_alarm_pf_synd1 r1;
	union dlb2_sys_alarm_pf_synd2 r2;
	bool valid;
	int i;

	r0.val = DLB2_CSR_RD(hw, DLB2_SYS_ALARM_PF_SYND0);

	valid = r0.field.valid;

	if (r0.field.valid) {
		r1.val = DLB2_CSR_RD(hw, DLB2_SYS_ALARM_PF_SYND1);
		r2.val = DLB2_CSR_RD(hw, DLB2_SYS_ALARM_PF_SYND2);

		dlb2_log_pf_vf_syndrome(hw,
					"PF Ingress error alarm",
					r0, r1, r2);

		dlb2_clear_syndrome_register(hw, DLB2_SYS_ALARM_PF_SYND0);

		dlb2_process_ingress_error(hw, r0, false, 0);
	}

	for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++) {
		r0.val = DLB2_CSR_RD(hw, DLB2_SYS_ALARM_VF_SYND0(i));

		valid |= r0.field.valid;

		if (!r0.field.valid)
			continue;

		r1.val = DLB2_CSR_RD(hw, DLB2_SYS_ALARM_VF_SYND1(i));
		r2.val = DLB2_CSR_RD(hw, DLB2_SYS_ALARM_VF_SYND2(i));

		dlb2_log_pf_vf_syndrome(hw,
					"VF Ingress error alarm",
					r0, r1, r2);

		dlb2_clear_syndrome_register(hw,
					     DLB2_SYS_ALARM_VF_SYND0(i));

		dlb2_process_ingress_error(hw, r0, true, i);
	}

	return valid;
}

int dlb2_get_group_sequence_numbers(struct dlb2_hw *hw, unsigned int group_id)
{
	if (group_id >= DLB2_MAX_NUM_SEQUENCE_NUMBER_GROUPS)
		return -EINVAL;

	return hw->rsrcs.sn_groups[group_id].sequence_numbers_per_queue;
}

int dlb2_get_group_sequence_number_occupancy(struct dlb2_hw *hw,
					     unsigned int group_id)
{
	if (group_id >= DLB2_MAX_NUM_SEQUENCE_NUMBER_GROUPS)
		return -EINVAL;

	return dlb2_sn_group_used_slots(&hw->rsrcs.sn_groups[group_id]);
}

static void dlb2_log_set_group_sequence_numbers(struct dlb2_hw *hw,
						unsigned int group_id,
						unsigned long val)
{
	DLB2_BASE_DBG(hw, "DLB2 set group sequence numbers:\n");
	DLB2_BASE_DBG(hw, "\tGroup ID: %u\n", group_id);
	DLB2_BASE_DBG(hw, "\tValue:    %lu\n", val);
}

int dlb2_set_group_sequence_numbers(struct dlb2_hw *hw,
				    unsigned int group_id,
				    unsigned long val)
{
	u32 valid_allocations[] = {64, 128, 256, 512, 1024};
	union dlb2_ro_pipe_grp_sn_mode r0 = { {0} };
	struct dlb2_sn_group *group;
	int mode;

	if (group_id >= DLB2_MAX_NUM_SEQUENCE_NUMBER_GROUPS)
		return -EINVAL;

	group = &hw->rsrcs.sn_groups[group_id];

	/* Once the first load-balanced queue using an SN group is configured,
	 * the group cannot be changed.
	 */
	if (group->slot_use_bitmap != 0)
		return -EPERM;

	for (mode = 0; mode < DLB2_MAX_NUM_SEQUENCE_NUMBER_MODES; mode++)
		if (val == valid_allocations[mode])
			break;

	if (mode == DLB2_MAX_NUM_SEQUENCE_NUMBER_MODES)
		return -EINVAL;

	group->mode = mode;
	group->sequence_numbers_per_queue = val;

	r0.field.sn_mode_0 = hw->rsrcs.sn_groups[0].mode;
	r0.field.sn_mode_1 = hw->rsrcs.sn_groups[1].mode;

	DLB2_CSR_WR(hw, DLB2_RO_PIPE_GRP_SN_MODE, r0.val);

	dlb2_log_set_group_sequence_numbers(hw, group_id, val);

	return 0;
}

static u32 dlb2_ldb_cq_inflight_count(struct dlb2_hw *hw,
				      struct dlb2_ldb_port *port)
{
	union dlb2_lsp_cq_ldb_infl_cnt r0;

	r0.val = DLB2_CSR_RD(hw, DLB2_LSP_CQ_LDB_INFL_CNT(port->id.phys_id));

	return r0.field.count;
}

static u32 dlb2_ldb_cq_token_count(struct dlb2_hw *hw,
				   struct dlb2_ldb_port *port)
{
	union dlb2_lsp_cq_ldb_tkn_cnt r0;

	r0.val = DLB2_CSR_RD(hw, DLB2_LSP_CQ_LDB_TKN_CNT(port->id.phys_id));

	/* Account for the initial token count, which is used in order to
	 * provide a CQ with depth less than 8.
	 */

	return r0.field.token_count - port->init_tkn_cnt;
}

static int dlb2_drain_ldb_cq(struct dlb2_hw *hw, struct dlb2_ldb_port *port)
{
	u32 infl_cnt, tkn_cnt;
	unsigned int i;

	infl_cnt = dlb2_ldb_cq_inflight_count(hw, port);
	tkn_cnt = dlb2_ldb_cq_token_count(hw, port);

	if (infl_cnt || tkn_cnt) {
		struct dlb2_hcw hcw_mem[8], *hcw;
		void __iomem *pp_addr;

		pp_addr = os_map_producer_port(hw, port->id.phys_id, true);

		/* Point hcw to a 64B-aligned location */
		hcw = (struct dlb2_hcw *)((uintptr_t)&hcw_mem[4] & ~0x3F);

		/* Program the first HCW for a completion and token return and
		 * the other HCWs as NOOPS
		 */

		memset(hcw, 0, 4 * sizeof(*hcw));
		hcw->qe_comp = (infl_cnt > 0);
		hcw->cq_token = (tkn_cnt > 0);
		hcw->lock_id = tkn_cnt - 1;

		/* Return tokens in the first HCW */
		os_enqueue_four_hcws(hw, hcw, pp_addr);

		hcw->cq_token = 0;

		/* Issue remaining completions (if any) */
		for (i = 1; i < infl_cnt; i++)
			os_enqueue_four_hcws(hw, hcw, pp_addr);

		os_fence_hcw(hw, pp_addr);

		os_unmap_producer_port(hw, pp_addr);
	}

	return 0;
}

static int dlb2_domain_wait_for_ldb_cqs_to_empty(struct dlb2_hw *hw,
						 struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_ldb_port *port;
	int i;

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i], port, iter) {
			int i;

			for (i = 0; i < DLB2_MAX_CQ_COMP_CHECK_LOOPS; i++) {
				if (dlb2_ldb_cq_inflight_count(hw, port) == 0)
					break;
			}

			if (i == DLB2_MAX_CQ_COMP_CHECK_LOOPS) {
				DLB2_BASE_ERR(hw,
					      "[%s()] Internal error: failed to flush load-balanced port %d's completions.\n",
					      __func__, port->id.phys_id);
				return -EFAULT;
			}
		}
	}

	return 0;
}

static int dlb2_domain_reset_software_state(struct dlb2_hw *hw,
					    struct dlb2_domain *domain)
{
	struct dlb2_dir_pq_pair *tmp_dir_port __attribute__((unused));
	struct dlb2_ldb_queue *tmp_ldb_queue __attribute__((unused));
	struct dlb2_ldb_port *tmp_ldb_port __attribute__((unused));
	struct dlb2_list_entry *iter1 __attribute__((unused));
	struct dlb2_list_entry *iter2 __attribute__((unused));
	struct dlb2_function_resources *rsrcs;
	struct dlb2_dir_pq_pair *dir_port;
	struct dlb2_ldb_queue *ldb_queue;
	struct dlb2_ldb_port *ldb_port;
	struct dlb2_list_head *list;
	int ret, i;

	rsrcs = domain->parent_func;

	/* Move the domain's ldb queues to the function's avail list */
	list = &domain->used_ldb_queues;
	DLB2_DOM_LIST_FOR_SAFE(*list, ldb_queue, tmp_ldb_queue, iter1, iter2) {
		if (ldb_queue->sn_cfg_valid) {
			struct dlb2_sn_group *grp;

			grp = &hw->rsrcs.sn_groups[ldb_queue->sn_group];

			dlb2_sn_group_free_slot(grp, ldb_queue->sn_slot);
			ldb_queue->sn_cfg_valid = false;
		}

		ldb_queue->owned = false;
		ldb_queue->num_mappings = 0;
		ldb_queue->num_pending_additions = 0;

		dlb2_list_del(&domain->used_ldb_queues,
			      &ldb_queue->domain_list);
		dlb2_list_add(&rsrcs->avail_ldb_queues,
			      &ldb_queue->func_list);
		rsrcs->num_avail_ldb_queues++;
	}

	list = &domain->avail_ldb_queues;
	DLB2_DOM_LIST_FOR_SAFE(*list, ldb_queue, tmp_ldb_queue, iter1, iter2) {
		ldb_queue->owned = false;

		dlb2_list_del(&domain->avail_ldb_queues,
			      &ldb_queue->domain_list);
		dlb2_list_add(&rsrcs->avail_ldb_queues,
			      &ldb_queue->func_list);
		rsrcs->num_avail_ldb_queues++;
	}

	/* Move the domain's ldb ports to the function's avail list */
	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		list = &domain->used_ldb_ports[i];
		DLB2_DOM_LIST_FOR_SAFE(*list, ldb_port, tmp_ldb_port,
				       iter1, iter2) {
			int j;

			ldb_port->owned = false;
			ldb_port->configured = false;
			ldb_port->num_pending_removals = 0;
			ldb_port->num_mappings = 0;
			for (j = 0; j < DLB2_MAX_NUM_QIDS_PER_LDB_CQ; j++)
				ldb_port->qid_map[j].state =
					DLB2_QUEUE_UNMAPPED;

			dlb2_list_del(&domain->used_ldb_ports[i],
				      &ldb_port->domain_list);
			dlb2_list_add(&rsrcs->avail_ldb_ports[i],
				      &ldb_port->func_list);
			rsrcs->num_avail_ldb_ports[i]++;
		}

		list = &domain->avail_ldb_ports[i];
		DLB2_DOM_LIST_FOR_SAFE(*list, ldb_port, tmp_ldb_port,
				       iter1, iter2) {
			ldb_port->owned = false;

			dlb2_list_del(&domain->avail_ldb_ports[i],
				      &ldb_port->domain_list);
			dlb2_list_add(&rsrcs->avail_ldb_ports[i],
				      &ldb_port->func_list);
			rsrcs->num_avail_ldb_ports[i]++;
		}
	}

	/* Move the domain's dir ports to the function's avail list */
	list = &domain->used_dir_pq_pairs;
	DLB2_DOM_LIST_FOR_SAFE(*list, dir_port, tmp_dir_port, iter1, iter2) {
		dir_port->owned = false;
		dir_port->port_configured = false;

		dlb2_list_del(&domain->used_dir_pq_pairs,
			      &dir_port->domain_list);

		dlb2_list_add(&rsrcs->avail_dir_pq_pairs,
			      &dir_port->func_list);
		rsrcs->num_avail_dir_pq_pairs++;
	}

	list = &domain->avail_dir_pq_pairs;
	DLB2_DOM_LIST_FOR_SAFE(*list, dir_port, tmp_dir_port, iter1, iter2) {
		dir_port->owned = false;

		dlb2_list_del(&domain->avail_dir_pq_pairs,
			      &dir_port->domain_list);

		dlb2_list_add(&rsrcs->avail_dir_pq_pairs,
			      &dir_port->func_list);
		rsrcs->num_avail_dir_pq_pairs++;
	}

	/* Return hist list entries to the function */
	ret = dlb2_bitmap_set_range(rsrcs->avail_hist_list_entries,
				    domain->hist_list_entry_base,
				    domain->total_hist_list_entries);
	if (ret) {
		DLB2_BASE_ERR(hw,
			      "[%s()] Internal error: domain hist list base doesn't match the function's bitmap.\n",
			      __func__);
		return ret;
	}

	domain->total_hist_list_entries = 0;
	domain->avail_hist_list_entries = 0;
	domain->hist_list_entry_base = 0;
	domain->hist_list_entry_offset = 0;

	rsrcs->num_avail_qed_entries += domain->num_ldb_credits;
	domain->num_ldb_credits = 0;

	rsrcs->num_avail_dqed_entries += domain->num_dir_credits;
	domain->num_dir_credits = 0;

	rsrcs->num_avail_aqed_entries += domain->num_avail_aqed_entries;
	rsrcs->num_avail_aqed_entries += domain->num_used_aqed_entries;
	domain->num_avail_aqed_entries = 0;
	domain->num_used_aqed_entries = 0;

	domain->num_pending_removals = 0;
	domain->num_pending_additions = 0;
	domain->configured = false;
	domain->started = false;

	/* Move the domain out of the used_domains list and back to the
	 * function's avail_domains list.
	 */
	dlb2_list_del(&rsrcs->used_domains, &domain->func_list);
	dlb2_list_add(&rsrcs->avail_domains, &domain->func_list);
	rsrcs->num_avail_domains++;

	return 0;
}

void dlb2_resource_reset(struct dlb2_hw *hw)
{
	struct dlb2_domain *domain, *next __attribute__((unused));
	struct dlb2_list_entry *iter1 __attribute__((unused));
	struct dlb2_list_entry *iter2 __attribute__((unused));
	int i;

	for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++) {
		DLB2_FUNC_LIST_FOR_SAFE(hw->vdev[i].used_domains, domain,
					next, iter1, iter2)
			dlb2_domain_reset_software_state(hw, domain);
	}

	DLB2_FUNC_LIST_FOR_SAFE(hw->pf.used_domains, domain,
				next, iter1, iter2)
		dlb2_domain_reset_software_state(hw, domain);
}

static u32 dlb2_dir_queue_depth(struct dlb2_hw *hw,
				struct dlb2_dir_pq_pair *queue)
{
	union dlb2_lsp_qid_dir_enqueue_cnt r0;

	r0.val = DLB2_CSR_RD(hw,
			     DLB2_LSP_QID_DIR_ENQUEUE_CNT(queue->id.phys_id));

	return r0.field.count;
}

static bool dlb2_dir_queue_is_empty(struct dlb2_hw *hw,
				    struct dlb2_dir_pq_pair *queue)
{
	return dlb2_dir_queue_depth(hw, queue) == 0;
}

int dlb2_hw_get_dir_queue_depth(struct dlb2_hw *hw,
				u32 domain_id,
				struct dlb2_get_dir_queue_depth_args *args,
				struct dlb2_cmd_response *resp,
				bool vdev_req,
				unsigned int vdev_id)
{
	struct dlb2_dir_pq_pair *queue;
	struct dlb2_domain *domain;
	int id;

	id = domain_id;

	domain = dlb2_get_domain_from_id(hw, id, vdev_req, vdev_id);
	if (!domain) {
		resp->status = DLB2_ST_INVALID_DOMAIN_ID;
		return -EINVAL;
	}

	id = args->queue_id;

	queue = dlb2_get_domain_used_dir_pq(id, vdev_req, domain);
	if (!queue) {
		resp->status = DLB2_ST_INVALID_QID;
		return -EINVAL;
	}

	resp->id = dlb2_dir_queue_depth(hw, queue);

	return 0;
}

static void
dlb2_log_pending_port_unmaps_args(struct dlb2_hw *hw,
				  struct dlb2_pending_port_unmaps_args *args,
				  bool vf_request,
				  unsigned int vf_id)
{
	DLB2_BASE_DBG(hw, "DLB unmaps in progress arguments:\n");
	if (vf_request)
		DLB2_BASE_DBG(hw, "(Request from VF %d)\n", vf_id);
	DLB2_BASE_DBG(hw, "\tPort ID: %d\n", args->port_id);
}

int dlb2_hw_pending_port_unmaps(struct dlb2_hw *hw,
				u32 domain_id,
				struct dlb2_pending_port_unmaps_args *args,
				struct dlb2_cmd_response *resp,
				bool vf_request,
				unsigned int vf_id)
{
	struct dlb2_domain *domain;
	struct dlb2_ldb_port *port;

	dlb2_log_pending_port_unmaps_args(hw, args, vf_request, vf_id);

	domain = dlb2_get_domain_from_id(hw, domain_id, vf_request, vf_id);

	if (!domain) {
		resp->status = DLB2_ST_INVALID_DOMAIN_ID;
		return -EINVAL;
	}

	port = dlb2_get_domain_used_ldb_port(args->port_id, vf_request, domain);
	if (!port || !port->configured) {
		resp->status = DLB2_ST_INVALID_PORT_ID;
		return -EINVAL;
	}

	resp->id = port->num_pending_removals;

	return 0;
}

static u32 dlb2_ldb_queue_depth(struct dlb2_hw *hw,
				struct dlb2_ldb_queue *queue)
{
	union dlb2_lsp_qid_aqed_active_cnt r0;
	union dlb2_lsp_qid_atm_active r1;
	union dlb2_lsp_qid_ldb_enqueue_cnt r2;

	r0.val = DLB2_CSR_RD(hw,
			     DLB2_LSP_QID_AQED_ACTIVE_CNT(queue->id.phys_id));
	r1.val = DLB2_CSR_RD(hw,
			     DLB2_LSP_QID_ATM_ACTIVE(queue->id.phys_id));

	r2.val = DLB2_CSR_RD(hw,
			     DLB2_LSP_QID_LDB_ENQUEUE_CNT(queue->id.phys_id));

	return r0.field.count + r1.field.count + r2.field.count;
}

static bool dlb2_ldb_queue_is_empty(struct dlb2_hw *hw,
				    struct dlb2_ldb_queue *queue)
{
	return dlb2_ldb_queue_depth(hw, queue) == 0;
}

int dlb2_hw_get_ldb_queue_depth(struct dlb2_hw *hw,
				u32 domain_id,
				struct dlb2_get_ldb_queue_depth_args *args,
				struct dlb2_cmd_response *resp,
				bool vf_req,
				unsigned int vdev_id)
{
	struct dlb2_ldb_queue *queue;
	struct dlb2_domain *domain;

	domain = dlb2_get_domain_from_id(hw, domain_id, vf_req, vdev_id);
	if (!domain) {
		resp->status = DLB2_ST_INVALID_DOMAIN_ID;
		return -EINVAL;
	}

	queue = dlb2_get_domain_ldb_queue(args->queue_id, vf_req, domain);
	if (!queue) {
		resp->status = DLB2_ST_INVALID_QID;
		return -EINVAL;
	}

	resp->id = dlb2_ldb_queue_depth(hw, queue);

	return 0;
}

static void __dlb2_domain_reset_ldb_port_registers(struct dlb2_hw *hw,
						   struct dlb2_ldb_port *port)
{
	DLB2_CSR_WR(hw,
		    DLB2_SYS_LDB_PP2VAS(port->id.phys_id),
		    DLB2_SYS_LDB_PP2VAS_RST);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_LDB_CQ2VAS(port->id.phys_id),
		    DLB2_CHP_LDB_CQ2VAS_RST);

	DLB2_CSR_WR(hw,
		    DLB2_SYS_LDB_PP2VDEV(port->id.phys_id),
		    DLB2_SYS_LDB_PP2VDEV_RST);

	if (port->id.vdev_owned) {
		unsigned int offs;
		u32 virt_id;

		/* DLB uses producer port address bits 17:12 to determine the
		 * producer port ID. In S-IOV mode, PP accesses come through
		 * the PF MMIO window for the physical producer port, so for
		 * translation purposes the virtual and physical port IDs are
		 * equal.
		 */
		if (hw->virt_mode == DLB2_VIRT_SRIOV)
			virt_id = port->id.virt_id;
		else
			virt_id = port->id.phys_id;

		offs = port->id.vdev_id * DLB2_MAX_NUM_LDB_PORTS + virt_id;

		DLB2_CSR_WR(hw,
			    DLB2_SYS_VF_LDB_VPP2PP(offs),
			    DLB2_SYS_VF_LDB_VPP2PP_RST);

		DLB2_CSR_WR(hw,
			    DLB2_SYS_VF_LDB_VPP_V(offs),
			    DLB2_SYS_VF_LDB_VPP_V_RST);
	}

	DLB2_CSR_WR(hw,
		    DLB2_SYS_LDB_PP_V(port->id.phys_id),
		    DLB2_SYS_LDB_PP_V_RST);

	DLB2_CSR_WR(hw,
		    DLB2_LSP_CQ_LDB_DSBL(port->id.phys_id),
		    DLB2_LSP_CQ_LDB_DSBL_RST);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_LDB_CQ_DEPTH(port->id.phys_id),
		    DLB2_CHP_LDB_CQ_DEPTH_RST);

	DLB2_CSR_WR(hw,
		    DLB2_LSP_CQ_LDB_INFL_LIM(port->id.phys_id),
		    DLB2_LSP_CQ_LDB_INFL_LIM_RST);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_HIST_LIST_LIM(port->id.phys_id),
		    DLB2_CHP_HIST_LIST_LIM_RST);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_HIST_LIST_BASE(port->id.phys_id),
		    DLB2_CHP_HIST_LIST_BASE_RST);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_HIST_LIST_POP_PTR(port->id.phys_id),
		    DLB2_CHP_HIST_LIST_POP_PTR_RST);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_HIST_LIST_PUSH_PTR(port->id.phys_id),
		    DLB2_CHP_HIST_LIST_PUSH_PTR_RST);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_LDB_CQ_INT_DEPTH_THRSH(port->id.phys_id),
		    DLB2_CHP_LDB_CQ_INT_DEPTH_THRSH_RST);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_LDB_CQ_TMR_THRSH(port->id.phys_id),
		    DLB2_CHP_LDB_CQ_TMR_THRSH_RST);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_LDB_CQ_INT_ENB(port->id.phys_id),
		    DLB2_CHP_LDB_CQ_INT_ENB_RST);

	DLB2_CSR_WR(hw,
		    DLB2_SYS_LDB_CQ_ISR(port->id.phys_id),
		    DLB2_SYS_LDB_CQ_ISR_RST);

	DLB2_CSR_WR(hw,
		    DLB2_LSP_CQ_LDB_TKN_DEPTH_SEL(port->id.phys_id),
		    DLB2_LSP_CQ_LDB_TKN_DEPTH_SEL_RST);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_LDB_CQ_TKN_DEPTH_SEL(port->id.phys_id),
		    DLB2_CHP_LDB_CQ_TKN_DEPTH_SEL_RST);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_LDB_CQ_WPTR(port->id.phys_id),
		    DLB2_CHP_LDB_CQ_WPTR_RST);

	DLB2_CSR_WR(hw,
		    DLB2_LSP_CQ_LDB_TKN_CNT(port->id.phys_id),
		    DLB2_LSP_CQ_LDB_TKN_CNT_RST);

	DLB2_CSR_WR(hw,
		    DLB2_SYS_LDB_CQ_ADDR_L(port->id.phys_id),
		    DLB2_SYS_LDB_CQ_ADDR_L_RST);

	DLB2_CSR_WR(hw,
		    DLB2_SYS_LDB_CQ_ADDR_U(port->id.phys_id),
		    DLB2_SYS_LDB_CQ_ADDR_U_RST);

	DLB2_CSR_WR(hw,
		    DLB2_SYS_LDB_CQ_AT(port->id.phys_id),
		    DLB2_SYS_LDB_CQ_AT_RST);

	DLB2_CSR_WR(hw,
		    DLB2_SYS_LDB_CQ_PASID(port->id.phys_id),
		    DLB2_SYS_LDB_CQ_PASID_RST);

	DLB2_CSR_WR(hw,
		    DLB2_SYS_LDB_CQ2VF_PF_RO(port->id.phys_id),
		    DLB2_SYS_LDB_CQ2VF_PF_RO_RST);

	DLB2_CSR_WR(hw,
		    DLB2_LSP_CQ_LDB_TOT_SCH_CNTL(port->id.phys_id),
		    DLB2_LSP_CQ_LDB_TOT_SCH_CNTL_RST);

	DLB2_CSR_WR(hw,
		    DLB2_LSP_CQ_LDB_TOT_SCH_CNTH(port->id.phys_id),
		    DLB2_LSP_CQ_LDB_TOT_SCH_CNTH_RST);

	DLB2_CSR_WR(hw,
		    DLB2_LSP_CQ2QID0(port->id.phys_id),
		    DLB2_LSP_CQ2QID0_RST);

	DLB2_CSR_WR(hw,
		    DLB2_LSP_CQ2QID1(port->id.phys_id),
		    DLB2_LSP_CQ2QID1_RST);

	DLB2_CSR_WR(hw,
		    DLB2_LSP_CQ2PRIOV(port->id.phys_id),
		    DLB2_LSP_CQ2PRIOV_RST);
}

static void dlb2_domain_reset_ldb_port_registers(struct dlb2_hw *hw,
						 struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_ldb_port *port;
	int i;

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i], port, iter)
			__dlb2_domain_reset_ldb_port_registers(hw, port);
	}
}

static void
__dlb2_domain_reset_dir_port_registers(struct dlb2_hw *hw,
				       struct dlb2_dir_pq_pair *port)
{
	DLB2_CSR_WR(hw,
		    DLB2_CHP_DIR_CQ2VAS(port->id.phys_id),
		    DLB2_CHP_DIR_CQ2VAS_RST);

	DLB2_CSR_WR(hw,
		    DLB2_LSP_CQ_DIR_DSBL(port->id.phys_id),
		    DLB2_LSP_CQ_DIR_DSBL_RST);

	DLB2_CSR_WR(hw, DLB2_SYS_DIR_CQ_OPT_CLR, port->id.phys_id);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_DIR_CQ_DEPTH(port->id.phys_id),
		    DLB2_CHP_DIR_CQ_DEPTH_RST);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_DIR_CQ_INT_DEPTH_THRSH(port->id.phys_id),
		    DLB2_CHP_DIR_CQ_INT_DEPTH_THRSH_RST);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_DIR_CQ_TMR_THRSH(port->id.phys_id),
		    DLB2_CHP_DIR_CQ_TMR_THRSH_RST);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_DIR_CQ_INT_ENB(port->id.phys_id),
		    DLB2_CHP_DIR_CQ_INT_ENB_RST);

	DLB2_CSR_WR(hw,
		    DLB2_SYS_DIR_CQ_ISR(port->id.phys_id),
		    DLB2_SYS_DIR_CQ_ISR_RST);

	DLB2_CSR_WR(hw,
		    DLB2_LSP_CQ_DIR_TKN_DEPTH_SEL_DSI(port->id.phys_id),
		    DLB2_LSP_CQ_DIR_TKN_DEPTH_SEL_DSI_RST);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_DIR_CQ_TKN_DEPTH_SEL(port->id.phys_id),
		    DLB2_CHP_DIR_CQ_TKN_DEPTH_SEL_RST);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_DIR_CQ_WPTR(port->id.phys_id),
		    DLB2_CHP_DIR_CQ_WPTR_RST);

	DLB2_CSR_WR(hw,
		    DLB2_LSP_CQ_DIR_TKN_CNT(port->id.phys_id),
		    DLB2_LSP_CQ_DIR_TKN_CNT_RST);

	DLB2_CSR_WR(hw,
		    DLB2_SYS_DIR_CQ_ADDR_L(port->id.phys_id),
		    DLB2_SYS_DIR_CQ_ADDR_L_RST);

	DLB2_CSR_WR(hw,
		    DLB2_SYS_DIR_CQ_ADDR_U(port->id.phys_id),
		    DLB2_SYS_DIR_CQ_ADDR_U_RST);

	DLB2_CSR_WR(hw,
		    DLB2_SYS_DIR_CQ_AT(port->id.phys_id),
		    DLB2_SYS_DIR_CQ_AT_RST);

	DLB2_CSR_WR(hw,
		    DLB2_SYS_DIR_CQ_PASID(port->id.phys_id),
		    DLB2_SYS_DIR_CQ_PASID_RST);

	DLB2_CSR_WR(hw,
		    DLB2_SYS_DIR_CQ_FMT(port->id.phys_id),
		    DLB2_SYS_DIR_CQ_FMT_RST);

	DLB2_CSR_WR(hw,
		    DLB2_SYS_DIR_CQ2VF_PF_RO(port->id.phys_id),
		    DLB2_SYS_DIR_CQ2VF_PF_RO_RST);

	DLB2_CSR_WR(hw,
		    DLB2_LSP_CQ_DIR_TOT_SCH_CNTL(port->id.phys_id),
		    DLB2_LSP_CQ_DIR_TOT_SCH_CNTL_RST);

	DLB2_CSR_WR(hw,
		    DLB2_LSP_CQ_DIR_TOT_SCH_CNTH(port->id.phys_id),
		    DLB2_LSP_CQ_DIR_TOT_SCH_CNTH_RST);

	DLB2_CSR_WR(hw,
		    DLB2_SYS_DIR_PP2VAS(port->id.phys_id),
		    DLB2_SYS_DIR_PP2VAS_RST);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_DIR_CQ2VAS(port->id.phys_id),
		    DLB2_CHP_DIR_CQ2VAS_RST);

	DLB2_CSR_WR(hw,
		    DLB2_SYS_DIR_PP2VDEV(port->id.phys_id),
		    DLB2_SYS_DIR_PP2VDEV_RST);

	if (port->id.vdev_owned) {
		unsigned int offs;
		u32 virt_id;

		/* DLB uses producer port address bits 17:12 to determine the
		 * producer port ID. In S-IOV mode, PP accesses come through
		 * the PF MMIO window for the physical producer port, so for
		 * translation purposes the virtual and physical port IDs are
		 * equal.
		 */
		if (hw->virt_mode == DLB2_VIRT_SRIOV)
			virt_id = port->id.virt_id;
		else
			virt_id = port->id.phys_id;

		offs = port->id.vdev_id * DLB2_MAX_NUM_DIR_PORTS + virt_id;

		DLB2_CSR_WR(hw,
			    DLB2_SYS_VF_DIR_VPP2PP(offs),
			    DLB2_SYS_VF_DIR_VPP2PP_RST);

		DLB2_CSR_WR(hw,
			    DLB2_SYS_VF_DIR_VPP_V(offs),
			    DLB2_SYS_VF_DIR_VPP_V_RST);
	}

	DLB2_CSR_WR(hw,
		    DLB2_SYS_DIR_PP_V(port->id.phys_id),
		    DLB2_SYS_DIR_PP_V_RST);
}

static void dlb2_domain_reset_dir_port_registers(struct dlb2_hw *hw,
						 struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_dir_pq_pair *port;

	DLB2_DOM_LIST_FOR(domain->used_dir_pq_pairs, port, iter)
		__dlb2_domain_reset_dir_port_registers(hw, port);
}

static void dlb2_domain_reset_ldb_queue_registers(struct dlb2_hw *hw,
						  struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_ldb_queue *queue;

	DLB2_DOM_LIST_FOR(domain->used_ldb_queues, queue, iter) {
		unsigned int queue_id = queue->id.phys_id;
		int i;

		DLB2_CSR_WR(hw,
			    DLB2_LSP_QID_NALDB_TOT_ENQ_CNTL(queue_id),
			    DLB2_LSP_QID_NALDB_TOT_ENQ_CNTL_RST);

		DLB2_CSR_WR(hw,
			    DLB2_LSP_QID_NALDB_TOT_ENQ_CNTH(queue_id),
			    DLB2_LSP_QID_NALDB_TOT_ENQ_CNTH_RST);

		DLB2_CSR_WR(hw,
			    DLB2_LSP_QID_ATM_TOT_ENQ_CNTL(queue_id),
			    DLB2_LSP_QID_ATM_TOT_ENQ_CNTL_RST);

		DLB2_CSR_WR(hw,
			    DLB2_LSP_QID_ATM_TOT_ENQ_CNTH(queue_id),
			    DLB2_LSP_QID_ATM_TOT_ENQ_CNTH_RST);

		DLB2_CSR_WR(hw,
			    DLB2_LSP_QID_NALDB_MAX_DEPTH(queue_id),
			    DLB2_LSP_QID_NALDB_MAX_DEPTH_RST);

		DLB2_CSR_WR(hw,
			    DLB2_LSP_QID_LDB_INFL_LIM(queue_id),
			    DLB2_LSP_QID_LDB_INFL_LIM_RST);

		DLB2_CSR_WR(hw,
			    DLB2_LSP_QID_AQED_ACTIVE_LIM(queue_id),
			    DLB2_LSP_QID_AQED_ACTIVE_LIM_RST);

		DLB2_CSR_WR(hw,
			    DLB2_LSP_QID_ATM_DEPTH_THRSH(queue_id),
			    DLB2_LSP_QID_ATM_DEPTH_THRSH_RST);

		DLB2_CSR_WR(hw,
			    DLB2_LSP_QID_NALDB_DEPTH_THRSH(queue_id),
			    DLB2_LSP_QID_NALDB_DEPTH_THRSH_RST);

		DLB2_CSR_WR(hw,
			    DLB2_SYS_LDB_QID_ITS(queue_id),
			    DLB2_SYS_LDB_QID_ITS_RST);

		DLB2_CSR_WR(hw,
			    DLB2_CHP_ORD_QID_SN(queue_id),
			    DLB2_CHP_ORD_QID_SN_RST);

		DLB2_CSR_WR(hw,
			    DLB2_CHP_ORD_QID_SN_MAP(queue_id),
			    DLB2_CHP_ORD_QID_SN_MAP_RST);

		DLB2_CSR_WR(hw,
			    DLB2_SYS_LDB_QID_V(queue_id),
			    DLB2_SYS_LDB_QID_V_RST);

		DLB2_CSR_WR(hw,
			    DLB2_SYS_LDB_QID_CFG_V(queue_id),
			    DLB2_SYS_LDB_QID_CFG_V_RST);

		if (queue->sn_cfg_valid) {
			u32 offs[2];

			offs[0] = DLB2_RO_PIPE_GRP_0_SLT_SHFT(queue->sn_slot);
			offs[1] = DLB2_RO_PIPE_GRP_1_SLT_SHFT(queue->sn_slot);

			DLB2_CSR_WR(hw,
				    offs[queue->sn_group],
				    DLB2_RO_PIPE_GRP_0_SLT_SHFT_RST);
		}

		for (i = 0; i < DLB2_LSP_QID2CQIDIX_NUM; i++) {
			DLB2_CSR_WR(hw,
				    DLB2_LSP_QID2CQIDIX(queue_id, i),
				    DLB2_LSP_QID2CQIDIX_00_RST);

			DLB2_CSR_WR(hw,
				    DLB2_LSP_QID2CQIDIX2(queue_id, i),
				    DLB2_LSP_QID2CQIDIX2_00_RST);

			DLB2_CSR_WR(hw,
				    DLB2_ATM_QID2CQIDIX(queue_id, i),
				    DLB2_ATM_QID2CQIDIX_00_RST);
		}
	}
}

static void dlb2_domain_reset_dir_queue_registers(struct dlb2_hw *hw,
						  struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_dir_pq_pair *queue;

	DLB2_DOM_LIST_FOR(domain->used_dir_pq_pairs, queue, iter) {
		DLB2_CSR_WR(hw,
			    DLB2_LSP_QID_DIR_MAX_DEPTH(queue->id.phys_id),
			    DLB2_LSP_QID_DIR_MAX_DEPTH_RST);

		DLB2_CSR_WR(hw,
			    DLB2_LSP_QID_DIR_TOT_ENQ_CNTL(queue->id.phys_id),
			    DLB2_LSP_QID_DIR_TOT_ENQ_CNTL_RST);

		DLB2_CSR_WR(hw,
			    DLB2_LSP_QID_DIR_TOT_ENQ_CNTH(queue->id.phys_id),
			    DLB2_LSP_QID_DIR_TOT_ENQ_CNTH_RST);

		DLB2_CSR_WR(hw,
			    DLB2_LSP_QID_DIR_DEPTH_THRSH(queue->id.phys_id),
			    DLB2_LSP_QID_DIR_DEPTH_THRSH_RST);

		DLB2_CSR_WR(hw,
			    DLB2_SYS_DIR_QID_ITS(queue->id.phys_id),
			    DLB2_SYS_DIR_QID_ITS_RST);

		DLB2_CSR_WR(hw,
			    DLB2_SYS_DIR_QID_V(queue->id.phys_id),
			    DLB2_SYS_DIR_QID_V_RST);
	}
}

static u32 dlb2_dir_cq_token_count(struct dlb2_hw *hw,
				   struct dlb2_dir_pq_pair *port)
{
	union dlb2_lsp_cq_dir_tkn_cnt r0;

	r0.val = DLB2_CSR_RD(hw, DLB2_LSP_CQ_DIR_TKN_CNT(port->id.phys_id));

	/* Account for the initial token count, which is used in order to
	 * provide a CQ with depth less than 8.
	 */

	return r0.field.count - port->init_tkn_cnt;
}

static int dlb2_domain_verify_reset_success(struct dlb2_hw *hw,
					    struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_dir_pq_pair *dir_port;
	struct dlb2_ldb_port *ldb_port;
	struct dlb2_ldb_queue *queue;
	int i;

	/* Confirm that all the domain's queue's inflight counts and AQED
	 * active counts are 0.
	 */
	DLB2_DOM_LIST_FOR(domain->used_ldb_queues, queue, iter) {
		if (!dlb2_ldb_queue_is_empty(hw, queue)) {
			DLB2_BASE_ERR(hw,
				      "[%s()] Internal error: failed to empty ldb queue %d\n",
				      __func__, queue->id.phys_id);
			return -EFAULT;
		}
	}

	/* Confirm that all the domain's CQs inflight and token counts are 0. */
	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i], ldb_port, iter) {
			if (dlb2_ldb_cq_inflight_count(hw, ldb_port) ||
			    dlb2_ldb_cq_token_count(hw, ldb_port)) {
				DLB2_BASE_ERR(hw,
					      "[%s()] Internal error: failed to empty ldb port %d\n",
					      __func__, ldb_port->id.phys_id);
				return -EFAULT;
			}
		}
	}

	DLB2_DOM_LIST_FOR(domain->used_dir_pq_pairs, dir_port, iter) {
		if (!dlb2_dir_queue_is_empty(hw, dir_port)) {
			DLB2_BASE_ERR(hw,
				      "[%s()] Internal error: failed to empty dir queue %d\n",
				      __func__, dir_port->id.phys_id);
			return -EFAULT;
		}

		if (dlb2_dir_cq_token_count(hw, dir_port)) {
			DLB2_BASE_ERR(hw,
				      "[%s()] Internal error: failed to empty dir port %d\n",
				      __func__, dir_port->id.phys_id);
			return -EFAULT;
		}
	}

	return 0;
}

static void dlb2_domain_reset_registers(struct dlb2_hw *hw,
					struct dlb2_domain *domain)
{
	dlb2_domain_reset_ldb_port_registers(hw, domain);

	dlb2_domain_reset_dir_port_registers(hw, domain);

	dlb2_domain_reset_ldb_queue_registers(hw, domain);

	dlb2_domain_reset_dir_queue_registers(hw, domain);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_CFG_LDB_VAS_CRD(domain->id.phys_id),
		    DLB2_CHP_CFG_LDB_VAS_CRD_RST);

	DLB2_CSR_WR(hw,
		    DLB2_CHP_CFG_DIR_VAS_CRD(domain->id.phys_id),
		    DLB2_CHP_CFG_DIR_VAS_CRD_RST);
}

static int dlb2_domain_drain_ldb_cqs(struct dlb2_hw *hw,
				     struct dlb2_domain *domain,
				     bool toggle_port)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_ldb_port *port;
	int ret, i;

	/* If the domain hasn't been started, there's no traffic to drain */
	if (!domain->started)
		return 0;

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i], port, iter) {
			if (toggle_port)
				dlb2_ldb_port_cq_disable(hw, port);

			ret = dlb2_drain_ldb_cq(hw, port);
			if (ret < 0)
				return ret;

			if (toggle_port)
				dlb2_ldb_port_cq_enable(hw, port);
		}
	}

	return 0;
}

static bool dlb2_domain_mapped_queues_empty(struct dlb2_hw *hw,
					    struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_ldb_queue *queue;

	DLB2_DOM_LIST_FOR(domain->used_ldb_queues, queue, iter) {
		if (queue->num_mappings == 0)
			continue;

		if (!dlb2_ldb_queue_is_empty(hw, queue))
			return false;
	}

	return true;
}

static int dlb2_domain_drain_mapped_queues(struct dlb2_hw *hw,
					   struct dlb2_domain *domain)
{
	int i, ret;

	/* If the domain hasn't been started, there's no traffic to drain */
	if (!domain->started)
		return 0;

	if (domain->num_pending_removals > 0) {
		DLB2_BASE_ERR(hw,
			      "[%s()] Internal error: failed to unmap domain queues\n",
			      __func__);
		return -EFAULT;
	}

	for (i = 0; i < DLB2_MAX_QID_EMPTY_CHECK_LOOPS; i++) {
		ret = dlb2_domain_drain_ldb_cqs(hw, domain, true);
		if (ret < 0)
			return ret;

		if (dlb2_domain_mapped_queues_empty(hw, domain))
			break;
	}

	if (i == DLB2_MAX_QID_EMPTY_CHECK_LOOPS) {
		DLB2_BASE_ERR(hw,
			      "[%s()] Internal error: failed to empty queues\n",
			      __func__);
		return -EFAULT;
	}

	/* Drain the CQs one more time. For the queues to go empty, they would
	 * have scheduled one or more QEs.
	 */
	ret = dlb2_domain_drain_ldb_cqs(hw, domain, true);
	if (ret < 0)
		return ret;

	return 0;
}

static int dlb2_domain_drain_unmapped_queue(struct dlb2_hw *hw,
					    struct dlb2_domain *domain,
					    struct dlb2_ldb_queue *queue)
{
	struct dlb2_ldb_port *port;
	int ret, i;

	/* If a domain has LDB queues, it must have LDB ports */
	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		if (!dlb2_list_empty(&domain->used_ldb_ports[i]))
			break;
	}

	if (i == DLB2_NUM_COS_DOMAINS) {
		DLB2_BASE_ERR(hw,
			      "[%s()] Internal error: No configured LDB ports\n",
			      __func__);
		return -EFAULT;
	}

	port = DLB2_DOM_LIST_HEAD(domain->used_ldb_ports[i], typeof(*port));

	/* If necessary, free up a QID slot in this CQ */
	if (port->num_mappings == DLB2_MAX_NUM_QIDS_PER_LDB_CQ) {
		struct dlb2_ldb_queue *mapped_queue;

		mapped_queue = &hw->rsrcs.ldb_queues[port->qid_map[0].qid];

		ret = dlb2_ldb_port_unmap_qid(hw, port, mapped_queue);
		if (ret)
			return ret;
	}

	ret = dlb2_ldb_port_map_qid_dynamic(hw, port, queue, 0);
	if (ret)
		return ret;

	return dlb2_domain_drain_mapped_queues(hw, domain);
}

static int dlb2_domain_drain_unmapped_queues(struct dlb2_hw *hw,
					     struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_ldb_queue *queue;
	int ret;

	/* If the domain hasn't been started, there's no traffic to drain */
	if (!domain->started)
		return 0;

	/* Pre-condition: the unattached queue must not have any outstanding
	 * completions. This is ensured by calling dlb2_domain_drain_ldb_cqs()
	 * prior to this in dlb2_domain_drain_mapped_queues().
	 */
	DLB2_DOM_LIST_FOR(domain->used_ldb_queues, queue, iter) {
		if (queue->num_mappings != 0 ||
		    dlb2_ldb_queue_is_empty(hw, queue))
			continue;

		ret = dlb2_domain_drain_unmapped_queue(hw, domain, queue);
		if (ret)
			return ret;
	}

	return 0;
}

static int dlb2_drain_dir_cq(struct dlb2_hw *hw,
			     struct dlb2_dir_pq_pair *port)
{
	unsigned int port_id = port->id.phys_id;
	u32 cnt;

	/* Return any outstanding tokens */
	cnt = dlb2_dir_cq_token_count(hw, port);

	if (cnt != 0) {
		struct dlb2_hcw hcw_mem[8], *hcw;
		void __iomem *pp_addr;

		pp_addr = os_map_producer_port(hw, port_id, false);

		/* Point hcw to a 64B-aligned location */
		hcw = (struct dlb2_hcw *)((uintptr_t)&hcw_mem[4] & ~0x3F);

		/* Program the first HCW for a batch token return and
		 * the rest as NOOPS
		 */
		memset(hcw, 0, 4 * sizeof(*hcw));
		hcw->cq_token = 1;
		hcw->lock_id = cnt - 1;

		os_enqueue_four_hcws(hw, hcw, pp_addr);

		os_fence_hcw(hw, pp_addr);

		os_unmap_producer_port(hw, pp_addr);
	}

	return 0;
}

static int dlb2_domain_drain_dir_cqs(struct dlb2_hw *hw,
				     struct dlb2_domain *domain,
				     bool toggle_port)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_dir_pq_pair *port;
	int ret;

	DLB2_DOM_LIST_FOR(domain->used_dir_pq_pairs, port, iter) {
		/* Can't drain a port if it's not configured, and there's
		 * nothing to drain if its queue is unconfigured.
		 */
		if (!port->port_configured || !port->queue_configured)
			continue;

		if (toggle_port)
			dlb2_dir_port_cq_disable(hw, port);

		ret = dlb2_drain_dir_cq(hw, port);
		if (ret < 0)
			return ret;

		if (toggle_port)
			dlb2_dir_port_cq_enable(hw, port);
	}

	return 0;
}

static bool dlb2_domain_dir_queues_empty(struct dlb2_hw *hw,
					 struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_dir_pq_pair *queue;

	DLB2_DOM_LIST_FOR(domain->used_dir_pq_pairs, queue, iter) {
		if (!dlb2_dir_queue_is_empty(hw, queue))
			return false;
	}

	return true;
}

static int dlb2_domain_drain_dir_queues(struct dlb2_hw *hw,
					struct dlb2_domain *domain)
{
	int i, ret;

	/* If the domain hasn't been started, there's no traffic to drain */
	if (!domain->started)
		return 0;

	for (i = 0; i < DLB2_MAX_QID_EMPTY_CHECK_LOOPS; i++) {
		ret = dlb2_domain_drain_dir_cqs(hw, domain, true);
		if (ret < 0)
			return ret;

		if (dlb2_domain_dir_queues_empty(hw, domain))
			break;
	}

	if (i == DLB2_MAX_QID_EMPTY_CHECK_LOOPS) {
		DLB2_BASE_ERR(hw,
			      "[%s()] Internal error: failed to empty queues\n",
			      __func__);
		return -EFAULT;
	}

	/* Drain the CQs one more time. For the queues to go empty, they would
	 * have scheduled one or more QEs.
	 */
	ret = dlb2_domain_drain_dir_cqs(hw, domain, true);
	if (ret < 0)
		return ret;

	return 0;
}

static void dlb2_domain_disable_dir_producer_ports(struct dlb2_hw *hw,
						   struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_dir_pq_pair *port;
	union dlb2_sys_dir_pp_v r1;

	r1.field.pp_v = 0;

	DLB2_DOM_LIST_FOR(domain->used_dir_pq_pairs, port, iter)
		DLB2_CSR_WR(hw,
			    DLB2_SYS_DIR_PP_V(port->id.phys_id),
			    r1.val);
}

static void dlb2_domain_disable_ldb_producer_ports(struct dlb2_hw *hw,
						   struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	union dlb2_sys_ldb_pp_v r1;
	struct dlb2_ldb_port *port;
	int i;

	r1.field.pp_v = 0;

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i], port, iter)
			DLB2_CSR_WR(hw,
				    DLB2_SYS_LDB_PP_V(port->id.phys_id),
				    r1.val);
	}
}

static void dlb2_domain_disable_dir_vpps(struct dlb2_hw *hw,
					 struct dlb2_domain *domain,
					 unsigned int vdev_id)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	union dlb2_sys_vf_dir_vpp_v r1;
	struct dlb2_dir_pq_pair *port;

	r1.field.vpp_v = 0;

	DLB2_DOM_LIST_FOR(domain->used_dir_pq_pairs, port, iter) {
		unsigned int offs;
		u32 virt_id;

		if (hw->virt_mode == DLB2_VIRT_SRIOV)
			virt_id = port->id.virt_id;
		else
			virt_id = port->id.phys_id;

		offs = vdev_id * DLB2_MAX_NUM_DIR_PORTS + virt_id;

		DLB2_CSR_WR(hw, DLB2_SYS_VF_DIR_VPP_V(offs), r1.val);
	}
}

static void dlb2_domain_disable_ldb_vpps(struct dlb2_hw *hw,
					 struct dlb2_domain *domain,
					 unsigned int vdev_id)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	union dlb2_sys_vf_ldb_vpp_v r1;
	struct dlb2_ldb_port *port;
	int i;

	r1.field.vpp_v = 0;

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i], port, iter) {
			unsigned int offs;
			u32 virt_id;

			if (hw->virt_mode == DLB2_VIRT_SRIOV)
				virt_id = port->id.virt_id;
			else
				virt_id = port->id.phys_id;

			offs = vdev_id * DLB2_MAX_NUM_LDB_PORTS + virt_id;

			DLB2_CSR_WR(hw, DLB2_SYS_VF_LDB_VPP_V(offs), r1.val);
		}
	}
}

static void dlb2_domain_disable_ldb_seq_checks(struct dlb2_hw *hw,
					       struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	union dlb2_chp_sn_chk_enbl r1;
	struct dlb2_ldb_port *port;
	int i;

	r1.field.en = 0;

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i], port, iter)
			DLB2_CSR_WR(hw,
				    DLB2_CHP_SN_CHK_ENBL(port->id.phys_id),
				    r1.val);
	}
}

static void
dlb2_domain_disable_ldb_port_interrupts(struct dlb2_hw *hw,
					struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	union dlb2_chp_ldb_cq_int_enb r0 = { {0} };
	union dlb2_chp_ldb_cq_wd_enb r1 = { {0} };
	struct dlb2_ldb_port *port;
	int i;

	r0.field.en_tim = 0;
	r0.field.en_depth = 0;

	r1.field.wd_enable = 0;

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i], port, iter) {
			DLB2_CSR_WR(hw,
				    DLB2_CHP_LDB_CQ_INT_ENB(port->id.phys_id),
				    r0.val);

			DLB2_CSR_WR(hw,
				    DLB2_CHP_LDB_CQ_WD_ENB(port->id.phys_id),
				    r1.val);
		}
	}
}

static void
dlb2_domain_disable_dir_port_interrupts(struct dlb2_hw *hw,
					struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	union dlb2_chp_dir_cq_int_enb r0 = { {0} };
	union dlb2_chp_dir_cq_wd_enb r1 = { {0} };
	struct dlb2_dir_pq_pair *port;

	r0.field.en_tim = 0;
	r0.field.en_depth = 0;

	r1.field.wd_enable = 0;

	DLB2_DOM_LIST_FOR(domain->used_dir_pq_pairs, port, iter) {
		DLB2_CSR_WR(hw,
			    DLB2_CHP_DIR_CQ_INT_ENB(port->id.phys_id),
			    r0.val);

		DLB2_CSR_WR(hw,
			    DLB2_CHP_DIR_CQ_WD_ENB(port->id.phys_id),
			    r1.val);
	}
}

static void
dlb2_domain_disable_ldb_queue_write_perms(struct dlb2_hw *hw,
					  struct dlb2_domain *domain)
{
	int domain_offset = domain->id.phys_id * DLB2_MAX_NUM_LDB_QUEUES;
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_ldb_queue *queue;

	DLB2_DOM_LIST_FOR(domain->used_ldb_queues, queue, iter) {
		union dlb2_sys_ldb_vasqid_v r0 = { {0} };
		union dlb2_sys_ldb_qid2vqid r1 = { {0} };
		union dlb2_sys_vf_ldb_vqid_v r2 = { {0} };
		union dlb2_sys_vf_ldb_vqid2qid r3 = { {0} };
		int idx;

		idx = domain_offset + queue->id.phys_id;

		DLB2_CSR_WR(hw, DLB2_SYS_LDB_VASQID_V(idx), r0.val);

		if (queue->id.vdev_owned) {
			DLB2_CSR_WR(hw,
				    DLB2_SYS_LDB_QID2VQID(queue->id.phys_id),
				    r1.val);

			idx = queue->id.vdev_id * DLB2_MAX_NUM_LDB_QUEUES +
				queue->id.virt_id;

			DLB2_CSR_WR(hw,
				    DLB2_SYS_VF_LDB_VQID_V(idx),
				    r2.val);

			DLB2_CSR_WR(hw,
				    DLB2_SYS_VF_LDB_VQID2QID(idx),
				    r3.val);
		}
	}
}

static void
dlb2_domain_disable_dir_queue_write_perms(struct dlb2_hw *hw,
					  struct dlb2_domain *domain)
{
	int domain_offset = domain->id.phys_id * DLB2_MAX_NUM_DIR_PORTS;
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_dir_pq_pair *queue;

	DLB2_DOM_LIST_FOR(domain->used_dir_pq_pairs, queue, iter) {
		union dlb2_sys_dir_vasqid_v r0 = { {0} };
		union dlb2_sys_vf_dir_vqid_v r1 = { {0} };
		union dlb2_sys_vf_dir_vqid2qid r2 = { {0} };
		int idx;

		idx = domain_offset + queue->id.phys_id;

		DLB2_CSR_WR(hw, DLB2_SYS_DIR_VASQID_V(idx), r0.val);

		if (queue->id.vdev_owned) {
			idx = queue->id.vdev_id * DLB2_MAX_NUM_DIR_PORTS +
				queue->id.virt_id;

			DLB2_CSR_WR(hw,
				    DLB2_SYS_VF_DIR_VQID_V(idx),
				    r1.val);

			DLB2_CSR_WR(hw,
				    DLB2_SYS_VF_DIR_VQID2QID(idx),
				    r2.val);
		}
	}
}

static void dlb2_domain_disable_dir_cqs(struct dlb2_hw *hw,
					struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_dir_pq_pair *port;

	DLB2_DOM_LIST_FOR(domain->used_dir_pq_pairs, port, iter) {
		port->enabled = false;

		dlb2_dir_port_cq_disable(hw, port);
	}
}

static void dlb2_domain_disable_ldb_cqs(struct dlb2_hw *hw,
					struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_ldb_port *port;
	int i;

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i], port, iter) {
			port->enabled = false;

			dlb2_ldb_port_cq_disable(hw, port);
		}
	}
}

static void dlb2_domain_enable_ldb_cqs(struct dlb2_hw *hw,
				       struct dlb2_domain *domain)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_ldb_port *port;
	int i;

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i], port, iter) {
			port->enabled = true;

			dlb2_ldb_port_cq_enable(hw, port);
		}
	}
}

static void dlb2_log_reset_domain(struct dlb2_hw *hw,
				  u32 domain_id,
				  bool vdev_req,
				  unsigned int vdev_id)
{
	DLB2_BASE_DBG(hw, "DLB2 reset domain:\n");
	if (vdev_req)
		DLB2_BASE_DBG(hw, "(Request from vdev %d)\n", vdev_id);
	DLB2_BASE_DBG(hw, "\tDomain ID: %d\n", domain_id);
}

static int __dlb2_reset_domain(struct dlb2_hw *hw,
			       u32 domain_id,
			       bool vdev_req,
			       unsigned int vdev_id)
{
	struct dlb2_domain *domain;
	int ret;

	dlb2_log_reset_domain(hw, domain_id, vdev_req, vdev_id);

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);

	if (!domain || !domain->configured)
		return -EINVAL;

	/* Disable VPPs */
	if (vdev_req) {
		dlb2_domain_disable_dir_vpps(hw, domain, vdev_id);

		dlb2_domain_disable_ldb_vpps(hw, domain, vdev_id);
	}

	/* Disable CQ interrupts */
	dlb2_domain_disable_dir_port_interrupts(hw, domain);

	dlb2_domain_disable_ldb_port_interrupts(hw, domain);

	/* For each queue owned by this domain, disable its write permissions to
	 * cause any traffic sent to it to be dropped. Well-behaved software
	 * should not be sending QEs at this point.
	 */
	dlb2_domain_disable_dir_queue_write_perms(hw, domain);

	dlb2_domain_disable_ldb_queue_write_perms(hw, domain);

	/* Turn off completion tracking on all the domain's PPs. */
	dlb2_domain_disable_ldb_seq_checks(hw, domain);

	/* Disable the LDB CQs and drain them in order to complete the map and
	 * unmap procedures, which require zero CQ inflights and zero QID
	 * inflights respectively.
	 */
	dlb2_domain_disable_ldb_cqs(hw, domain);

	ret = dlb2_domain_drain_ldb_cqs(hw, domain, false);
	if (ret < 0)
		return ret;

	ret = dlb2_domain_wait_for_ldb_cqs_to_empty(hw, domain);
	if (ret < 0)
		return ret;

	ret = dlb2_domain_finish_unmap_qid_procedures(hw, domain);
	if (ret < 0)
		return ret;

	ret = dlb2_domain_finish_map_qid_procedures(hw, domain);
	if (ret < 0)
		return ret;

	/* Re-enable the CQs in order to drain the mapped queues. */
	dlb2_domain_enable_ldb_cqs(hw, domain);

	ret = dlb2_domain_drain_mapped_queues(hw, domain);
	if (ret < 0)
		return ret;

	ret = dlb2_domain_drain_unmapped_queues(hw, domain);
	if (ret < 0)
		return ret;

	/* Done draining LDB QEs, so disable the CQs. */
	dlb2_domain_disable_ldb_cqs(hw, domain);

	dlb2_domain_drain_dir_queues(hw, domain);

	/* Done draining DIR QEs, so disable the CQs. */
	dlb2_domain_disable_dir_cqs(hw, domain);

	/* Disable PPs */
	dlb2_domain_disable_dir_producer_ports(hw, domain);

	dlb2_domain_disable_ldb_producer_ports(hw, domain);

	ret = dlb2_domain_verify_reset_success(hw, domain);
	if (ret)
		return ret;

	/* Reset the QID and port state. */
	dlb2_domain_reset_registers(hw, domain);

	/* Hardware reset complete. Reset the domain's software state */
	ret = dlb2_domain_reset_software_state(hw, domain);
	if (ret)
		return ret;

	return 0;
}

/**
 * dlb2_reset_domain() - Reset a DLB scheduling domain and its associated
 *	hardware resources.
 * @hw:	  Contains the current state of the DLB2 hardware.
 * @args: User-provided arguments.
 * @resp: Response to user.
 *
 * Note: User software *must* stop sending to this domain's producer ports
 * before invoking this function, otherwise undefined behavior will result.
 *
 * Return: returns < 0 on error, 0 otherwise.
 */
int dlb2_reset_domain(struct dlb2_hw *hw,
		      u32 domain_id,
		      bool vdev_req,
		      unsigned int vdev_id)
{
	return __dlb2_reset_domain(hw, domain_id, vdev_req, vdev_id);
}

int dlb2_reset_vdev(struct dlb2_hw *hw, unsigned int id)
{
	struct dlb2_domain *domain, *next __attribute__((unused));
	struct dlb2_list_entry *it1 __attribute__((unused));
	struct dlb2_list_entry *it2 __attribute__((unused));
	struct dlb2_function_resources *rsrcs;

	if (id >= DLB2_MAX_NUM_VDEVS) {
		DLB2_BASE_ERR(hw,
			      "[%s()] Internal error: invalid vdev ID %d\n",
			      __func__, id);
		return -1;
	}

	rsrcs = &hw->vdev[id];

	DLB2_FUNC_LIST_FOR_SAFE(rsrcs->used_domains, domain, next, it1, it2) {
		int ret = __dlb2_reset_domain(hw,
					      domain->id.virt_id,
					      true,
					      id);
		if (ret)
			return ret;
	}

	return 0;
}

int dlb2_ldb_port_owned_by_domain(struct dlb2_hw *hw,
				  u32 domain_id,
				  u32 port_id,
				  bool vdev_req,
				  unsigned int vdev_id)
{
	struct dlb2_ldb_port *port;
	struct dlb2_domain *domain;

	if (vdev_req && vdev_id >= DLB2_MAX_NUM_VDEVS)
		return -EINVAL;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);

	if (!domain || !domain->configured)
		return -EINVAL;

	port = dlb2_get_domain_ldb_port(port_id, vdev_req, domain);

	if (!port)
		return -EINVAL;

	return port->domain_id.phys_id == domain->id.phys_id;
}

int dlb2_dir_port_owned_by_domain(struct dlb2_hw *hw,
				  u32 domain_id,
				  u32 port_id,
				  bool vdev_req,
				  unsigned int vdev_id)
{
	struct dlb2_dir_pq_pair *port;
	struct dlb2_domain *domain;

	if (vdev_req && vdev_id >= DLB2_MAX_NUM_VDEVS)
		return -EINVAL;

	domain = dlb2_get_domain_from_id(hw, domain_id, vdev_req, vdev_id);

	if (!domain || !domain->configured)
		return -EINVAL;

	port = dlb2_get_domain_dir_pq(port_id, vdev_req, domain);

	if (!port)
		return -EINVAL;

	return port->domain_id.phys_id == domain->id.phys_id;
}

static inline bool dlb2_ldb_port_owned_by_vf(struct dlb2_hw *hw,
					     u32 vdev_id,
					     u32 port_id)
{
	return (hw->rsrcs.ldb_ports[port_id].id.vdev_owned &&
		hw->rsrcs.ldb_ports[port_id].id.vdev_id == vdev_id);
}

static inline bool dlb2_dir_port_owned_by_vf(struct dlb2_hw *hw,
					     u32 vdev_id,
					     u32 port_id)
{
	return (hw->rsrcs.dir_pq_pairs[port_id].id.vdev_owned &&
		hw->rsrcs.dir_pq_pairs[port_id].id.vdev_id == vdev_id);
}

int dlb2_hw_get_num_resources(struct dlb2_hw *hw,
			      struct dlb2_get_num_resources_args *arg,
			      bool vdev_req,
			      unsigned int vdev_id)
{
	struct dlb2_function_resources *rsrcs;
	struct dlb2_bitmap *map;
	int i;

	if (vdev_req && vdev_id >= DLB2_MAX_NUM_VDEVS)
		return -EINVAL;

	if (vdev_req)
		rsrcs = &hw->vdev[vdev_id];
	else
		rsrcs = &hw->pf;

	arg->num_sched_domains = rsrcs->num_avail_domains;

	arg->num_ldb_queues = rsrcs->num_avail_ldb_queues;

	arg->num_ldb_ports = 0;
	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++)
		arg->num_ldb_ports += rsrcs->num_avail_ldb_ports[i];

	arg->num_cos0_ldb_ports = rsrcs->num_avail_ldb_ports[0];
	arg->num_cos1_ldb_ports = rsrcs->num_avail_ldb_ports[1];
	arg->num_cos2_ldb_ports = rsrcs->num_avail_ldb_ports[2];
	arg->num_cos3_ldb_ports = rsrcs->num_avail_ldb_ports[3];

	arg->num_dir_ports = rsrcs->num_avail_dir_pq_pairs;

	arg->num_atomic_inflights = rsrcs->num_avail_aqed_entries;

	map = rsrcs->avail_hist_list_entries;

	arg->num_hist_list_entries = dlb2_bitmap_count(map);

	arg->max_contiguous_hist_list_entries =
		dlb2_bitmap_longest_set_range(map);

	arg->num_ldb_credits = rsrcs->num_avail_qed_entries;

	arg->num_dir_credits = rsrcs->num_avail_dqed_entries;

	return 0;
}

int dlb2_hw_get_num_used_resources(struct dlb2_hw *hw,
				   struct dlb2_get_num_resources_args *arg,
				   bool vdev_req,
				   unsigned int vdev_id)
{
	struct dlb2_list_entry *iter1 __attribute__((unused));
	struct dlb2_list_entry *iter2 __attribute__((unused));
	struct dlb2_function_resources *rsrcs;
	struct dlb2_domain *domain;

	if (vdev_req && vdev_id >= DLB2_MAX_NUM_VDEVS)
		return -EINVAL;

	rsrcs = (vdev_req) ? &hw->vdev[vdev_id] : &hw->pf;

	memset(arg, 0, sizeof(struct dlb2_get_num_resources_args));

	DLB2_FUNC_LIST_FOR(rsrcs->used_domains, domain, iter1) {
		struct dlb2_ldb_queue *queue;
		struct dlb2_ldb_port *ldb_port;
		struct dlb2_dir_pq_pair *dir_port;
		int i;

		arg->num_sched_domains++;

		arg->num_atomic_inflights += domain->num_used_aqed_entries;

		DLB2_DOM_LIST_FOR(domain->used_ldb_queues, queue, iter2)
			arg->num_ldb_queues++;
		DLB2_DOM_LIST_FOR(domain->avail_ldb_queues, queue, iter2)
			arg->num_ldb_queues++;

		for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
			DLB2_DOM_LIST_FOR(domain->used_ldb_ports[i],
					  ldb_port, iter2)
				arg->num_ldb_ports++;
			DLB2_DOM_LIST_FOR(domain->avail_ldb_ports[i],
					  ldb_port, iter2)
				arg->num_ldb_ports++;
		}

		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[0], ldb_port, iter2)
			arg->num_cos0_ldb_ports++;
		DLB2_DOM_LIST_FOR(domain->avail_ldb_ports[0], ldb_port, iter2)
			arg->num_cos0_ldb_ports++;

		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[1], ldb_port, iter2)
			arg->num_cos1_ldb_ports++;
		DLB2_DOM_LIST_FOR(domain->avail_ldb_ports[1], ldb_port, iter2)
			arg->num_cos1_ldb_ports++;

		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[2], ldb_port, iter2)
			arg->num_cos2_ldb_ports++;
		DLB2_DOM_LIST_FOR(domain->avail_ldb_ports[2], ldb_port, iter2)
			arg->num_cos2_ldb_ports++;

		DLB2_DOM_LIST_FOR(domain->used_ldb_ports[3], ldb_port, iter2)
			arg->num_cos3_ldb_ports++;
		DLB2_DOM_LIST_FOR(domain->avail_ldb_ports[3], ldb_port, iter2)
			arg->num_cos3_ldb_ports++;

		DLB2_DOM_LIST_FOR(domain->used_dir_pq_pairs, dir_port, iter2)
			arg->num_dir_ports++;
		DLB2_DOM_LIST_FOR(domain->avail_dir_pq_pairs, dir_port, iter2)
			arg->num_dir_ports++;

		arg->num_ldb_credits += domain->num_ldb_credits;

		arg->num_dir_credits += domain->num_dir_credits;

		arg->num_hist_list_entries += domain->total_hist_list_entries;
	}

	return 0;
}

#define DLB2_SMON_NUM_MODES 6
#define DLB2_SMON_NUM_COMP_MODES 2
#define DLB2_SMON_NUM_CAP_MODES 5

static int
dlb2_verify_write_smon_args(struct dlb2_hw *hw,
			    struct dlb2_write_smon_args *args,
			    struct dlb2_cmd_response *resp)
{
	if (args->smon_id >= NUM_DLB2_SMONS) {
		resp->status = DLB2_ST_INVALID_SMON_ID;
		return -EINVAL;
	}

	if (args->mode >= DLB2_SMON_NUM_MODES) {
		resp->status = DLB2_ST_INVALID_SMON_MODE;
		return -EINVAL;
	}

	if (args->cmpm0 >= DLB2_SMON_NUM_COMP_MODES) {
		resp->status = DLB2_ST_INVALID_SMON_COMP_MODE;
		return -EINVAL;
	}

	if (args->cmpm1 >= DLB2_SMON_NUM_COMP_MODES) {
		resp->status = DLB2_ST_INVALID_SMON_COMP_MODE;
		return -EINVAL;
	}

	if (args->capm0 >= DLB2_SMON_NUM_CAP_MODES) {
		resp->status = DLB2_ST_INVALID_SMON_CAP_MODE;
		return -EINVAL;
	}

	if (args->capm1 >= DLB2_SMON_NUM_CAP_MODES) {
		resp->status = DLB2_ST_INVALID_SMON_CAP_MODE;
		return -EINVAL;
	}

	return 0;
}

static int
dlb2_verify_read_smon_args(struct dlb2_hw *hw,
			   struct dlb2_read_smon_args *args,
			   struct dlb2_cmd_response *resp)
{
	if (args->smon_id >= NUM_DLB2_SMONS) {
		resp->status = DLB2_ST_INVALID_SMON_ID;
		return -EINVAL;
	}

	return 0;
}

struct dlb2_smon_addr {
	u32 activitycntr0;
	u32 activitycntr1;
	u32 compare0;
	u32 compare1;
	u32 cfg0;
	u32 cfg1;
	u32 comp_mask0;
	u32 comp_mask1;
	u32 max_tmr;
	u32 tmr;
};

static struct dlb2_smon_addr dlb2_smon_addrs[NUM_DLB2_SMONS] = {
	[DLB2_SMON_ATM] = {
		DLB2_ATM_SMON_ACTIVITYCNTR0,
		DLB2_ATM_SMON_ACTIVITYCNTR1,
		DLB2_ATM_SMON_COMPARE0,
		DLB2_ATM_SMON_COMPARE1,
		DLB2_ATM_SMON_CFG0,
		DLB2_ATM_SMON_CFG1,
		0,
		0,
		DLB2_ATM_SMON_MAX_TMR,
		DLB2_ATM_SMON_TMR,
	},
	[DLB2_SMON_CHP] = {
		DLB2_CHP_SMON_ACTIVITYCNTR0,
		DLB2_CHP_SMON_ACTIVITYCNTR1,
		DLB2_CHP_SMON_COMPARE0,
		DLB2_CHP_SMON_COMPARE1,
		DLB2_CHP_SMON_CFG0,
		DLB2_CHP_SMON_CFG1,
		0,
		0,
		DLB2_CHP_SMON_MAX_TMR,
		DLB2_CHP_SMON_TMR,
	},
	[DLB2_SMON_PM] = {
		DLB2_SYS_PM_SMON_ACTIVITYCNTR0,
		DLB2_SYS_PM_SMON_ACTIVITYCNTR1,
		DLB2_SYS_PM_SMON_COMPARE0,
		DLB2_SYS_PM_SMON_COMPARE1,
		DLB2_SYS_PM_SMON_CFG0,
		DLB2_SYS_PM_SMON_CFG1,
		DLB2_SYS_PM_SMON_COMP_MASK0,
		DLB2_SYS_PM_SMON_COMP_MASK1,
		DLB2_SYS_PM_SMON_MAX_TMR,
		DLB2_SYS_PM_SMON_TMR,
	},
	[DLB2_SMON_DQED] = {
		DLB2_DQED_PIPE_SMON_ACTIVITYCNTR0,
		DLB2_DQED_PIPE_SMON_ACTIVITYCNTR1,
		DLB2_DQED_PIPE_SMON_COMPARE0,
		DLB2_DQED_PIPE_SMON_COMPARE1,
		DLB2_DQED_PIPE_SMON_CFG0,
		DLB2_DQED_PIPE_SMON_CFG1,
		0,
		0,
		DLB2_DQED_PIPE_SMON_MAX_TMR,
		DLB2_DQED_PIPE_SMON_TMR,
	},
	[DLB2_SMON_LSP] = {
		DLB2_LSP_SMON_ACTIVITYCNTR0,
		DLB2_LSP_SMON_ACTIVITYCNTR1,
		DLB2_LSP_SMON_COMPARE0,
		DLB2_LSP_SMON_COMPARE1,
		DLB2_LSP_SMON_CFG0,
		DLB2_LSP_SMON_CFG1,
		0,
		0,
		DLB2_LSP_SMON_MAX_TMR,
		DLB2_LSP_SMON_TMR,
	},
	[DLB2_SMON_ROP] = {
		DLB2_RO_PIPE_SMON_ACTIVITYCNTR0,
		DLB2_RO_PIPE_SMON_ACTIVITYCNTR1,
		DLB2_RO_PIPE_SMON_COMPARE0,
		DLB2_RO_PIPE_SMON_COMPARE1,
		DLB2_RO_PIPE_SMON_CFG0,
		DLB2_RO_PIPE_SMON_CFG1,
		0,
		0,
		DLB2_RO_PIPE_SMON_MAX_TMR,
		DLB2_RO_PIPE_SMON_TMR,
	},
	[DLB2_SMON_DIR] = {
		DLB2_DP_SMON_ACTIVITYCNTR0,
		DLB2_DP_SMON_ACTIVITYCNTR1,
		DLB2_DP_SMON_COMPARE0,
		DLB2_DP_SMON_COMPARE1,
		DLB2_DP_SMON_CFG0,
		DLB2_DP_SMON_CFG1,
		0,
		0,
		DLB2_DP_SMON_MAX_TMR,
		DLB2_DP_SMON_TMR,
	},
	[DLB2_SMON_AQED] = {
		DLB2_AQED_PIPE_SMON_ACTIVITYCNTR0,
		DLB2_AQED_PIPE_SMON_ACTIVITYCNTR1,
		DLB2_AQED_PIPE_SMON_COMPARE0,
		DLB2_AQED_PIPE_SMON_COMPARE1,
		DLB2_AQED_PIPE_SMON_CFG0,
		DLB2_AQED_PIPE_SMON_CFG1,
		0,
		0,
		DLB2_AQED_PIPE_SMON_MAX_TMR,
		DLB2_AQED_PIPE_SMON_TMR,
	},
	[DLB2_SMON_QED] = {
		DLB2_QED_PIPE_SMON_ACTIVITYCNTR0,
		DLB2_QED_PIPE_SMON_ACTIVITYCNTR1,
		DLB2_QED_PIPE_SMON_COMPARE0,
		DLB2_QED_PIPE_SMON_COMPARE1,
		DLB2_QED_PIPE_SMON_CFG0,
		DLB2_QED_PIPE_SMON_CFG1,
		0,
		0,
		DLB2_QED_PIPE_SMON_MAX_TMR,
		DLB2_QED_PIPE_SMON_TMR,
	},
	[DLB2_SMON_NALB] = {
		DLB2_NALB_PIPE_SMON_ACTIVITYCNTR0,
		DLB2_NALB_PIPE_SMON_ACTIVITYCNTR1,
		DLB2_NALB_PIPE_SMON_COMPARE0,
		DLB2_NALB_PIPE_SMON_COMPARE1,
		DLB2_NALB_PIPE_SMON_CFG0,
		DLB2_NALB_PIPE_SMON_CFG1,
		0,
		0,
		DLB2_NALB_PIPE_SMON_MAX_TMR,
		DLB2_NALB_PIPE_SMON_TMR,
	},
	[DLB2_SMON_SYS0] = {
		DLB2_SYS_SMON_ACTIVITYCNTR0(0),
		DLB2_SYS_SMON_ACTIVITYCNTR1(0),
		DLB2_SYS_SMON_COMPARE0(0),
		DLB2_SYS_SMON_COMPARE1(0),
		DLB2_SYS_SMON_CFG0(0),
		DLB2_SYS_SMON_CFG1(0),
		DLB2_SYS_SMON_COMP_MASK0(0),
		DLB2_SYS_SMON_COMP_MASK1(0),
		DLB2_SYS_SMON_MAX_TMR(0),
		DLB2_SYS_SMON_TMR(0),
	},
	[DLB2_SMON_SYS1] = {
		DLB2_SYS_SMON_ACTIVITYCNTR0(1),
		DLB2_SYS_SMON_ACTIVITYCNTR1(1),
		DLB2_SYS_SMON_COMPARE0(1),
		DLB2_SYS_SMON_COMPARE1(1),
		DLB2_SYS_SMON_CFG0(1),
		DLB2_SYS_SMON_CFG1(1),
		DLB2_SYS_SMON_COMP_MASK0(1),
		DLB2_SYS_SMON_COMP_MASK1(1),
		DLB2_SYS_SMON_MAX_TMR(1),
		DLB2_SYS_SMON_TMR(1),
	},
	[DLB2_SMON_IOSF0] = {
		DLB2_IOSF_SMON_ACTIVITYCNTR0(0),
		DLB2_IOSF_SMON_ACTIVITYCNTR1(0),
		DLB2_IOSF_SMON_COMPARE0(0),
		DLB2_IOSF_SMON_COMPARE1(0),
		DLB2_IOSF_SMON_CFG0(0),
		DLB2_IOSF_SMON_CFG1(0),
		DLB2_IOSF_SMON_COMP_MASK0(0),
		DLB2_IOSF_SMON_COMP_MASK1(0),
		DLB2_IOSF_SMON_MAX_TMR(0),
		DLB2_IOSF_SMON_TMR(0),
	},
	[DLB2_SMON_IOSF1] = {
		DLB2_IOSF_SMON_ACTIVITYCNTR0(1),
		DLB2_IOSF_SMON_ACTIVITYCNTR1(1),
		DLB2_IOSF_SMON_COMPARE0(1),
		DLB2_IOSF_SMON_COMPARE1(1),
		DLB2_IOSF_SMON_CFG0(1),
		DLB2_IOSF_SMON_CFG1(1),
		DLB2_IOSF_SMON_COMP_MASK0(1),
		DLB2_IOSF_SMON_COMP_MASK1(1),
		DLB2_IOSF_SMON_MAX_TMR(1),
		DLB2_IOSF_SMON_TMR(1),
	},
};

int dlb2_write_smon(struct dlb2_hw *hw,
		    struct dlb2_write_smon_args *args,
		    struct dlb2_cmd_response *resp)
{
	union dlb2_sys_smon_cfg0 r0 = {0};
	union dlb2_sys_smon_cfg1 r1 = {0};
	union dlb2_sys_smon_compare0 r2 = {0};
	union dlb2_sys_smon_compare1 r3 = {0};
	union dlb2_sys_smon_comp_mask0 r4 = {0};
	union dlb2_sys_smon_comp_mask1 r5 = {0};
	union dlb2_sys_smon_activitycntr0 r6 = {0};
	union dlb2_sys_smon_activitycntr1 r7 = {0};
	union dlb2_sys_smon_tmr r8 = {0};
	union dlb2_sys_smon_max_tmr r9 = {0};
	struct dlb2_smon_addr *addr;
	int ret;

	ret = dlb2_verify_write_smon_args(hw, args, resp);
	if (ret)
		return ret;

	addr = &dlb2_smon_addrs[args->smon_id];

	/* Disable the SMON before configuring it */
	r0.val = DLB2_CSR_RD(hw, addr->cfg0);

	r0.field.smon_enable = 0;

	/* Disable the SMON before configuring it */
	DLB2_CSR_WR(hw, addr->cfg0, r0.val);

	r1.field.mode0 = args->sel0;
	r1.field.mode1 = args->sel1;

	DLB2_CSR_WR(hw, addr->cfg1, r1.val);

	r2.field.compare0 = args->cmpv0;

	DLB2_CSR_WR(hw, addr->compare0, r2.val);

	r3.field.compare1 = args->cmpv1;

	DLB2_CSR_WR(hw, addr->compare1, r3.val);

	if (args->smon_id == DLB2_SMON_SYS0 ||
	    args->smon_id == DLB2_SMON_SYS1 ||
	    args->smon_id == DLB2_SMON_IOSF0 ||
	    args->smon_id == DLB2_SMON_IOSF1 ||
	    args->smon_id == DLB2_SMON_PM) {
		r4.field.comp_mask0 = args->mask0;

		DLB2_CSR_WR(hw, addr->comp_mask0, r4.val);

		r5.field.comp_mask1 = args->mask1;

		DLB2_CSR_WR(hw, addr->comp_mask1, r5.val);
	}

	r6.field.counter0 = args->cnt0;

	DLB2_CSR_WR(hw, addr->activitycntr0, r6.val);

	r7.field.counter1 = args->cnt1;

	DLB2_CSR_WR(hw, addr->activitycntr1, r7.val);

	r8.field.timer_val = args->timer;

	DLB2_CSR_WR(hw, addr->tmr, r8.val);

	r9.field.maxvalue = args->max_timer;

	DLB2_CSR_WR(hw, addr->max_tmr, r9.val);

	if (args->smon_id == DLB2_SMON_LSP) {
		union dlb2_lsp_cfg_ctrl_general_0 r10;

		r10.val = DLB2_CSR_RD(hw, DLB2_LSP_CFG_CTRL_GENERAL_0);

		r10.field.smon0_valid_sel = args->lsp_valid_sel;
		r10.field.smon0_value_sel = args->lsp_value_sel;
		r10.field.smon0_compare_sel = args->lsp_compare_sel;

		DLB2_CSR_WR(hw, DLB2_LSP_CFG_CTRL_GENERAL_0, r10.val);
	}

	r0.val = DLB2_CSR_RD(hw, addr->cfg0);

	r0.field.smon_enable = args->en;
	r0.field.smon0_function = args->capm0;
	r0.field.smon0_function_compare = args->cmpm0;
	r0.field.smon1_function = args->capm1;
	r0.field.smon1_function_compare = args->cmpm1;
	r0.field.smon_mode = args->mode;
	r0.field.stopcounterovfl = args->haltc;
	r0.field.stoptimerovfl = args->haltt;

	/* Enable the SMON */
	DLB2_CSR_WR(hw, addr->cfg0, r0.val);

	return 0;
}

int dlb2_read_smon(struct dlb2_hw *hw,
		   struct dlb2_read_smon_args *args,
		   struct dlb2_cmd_response *resp)
{
	union dlb2_sys_smon_cfg0 r0;
	union dlb2_sys_smon_cfg1 r1;
	struct dlb2_smon_addr *addr;
	int ret;

	ret = dlb2_verify_read_smon_args(hw, args, resp);
	if (ret)
		return ret;

	addr = &dlb2_smon_addrs[args->smon_id];

	r0.val = DLB2_CSR_RD(hw, addr->cfg0);

	args->en = r0.field.smon_enable;
	args->capm0 = r0.field.smon0_function;
	args->cmpm0 = r0.field.smon0_function_compare;
	args->capm1 = r0.field.smon1_function;
	args->cmpm1 = r0.field.smon1_function_compare;
	args->mode = r0.field.smon_mode;
	args->haltc = r0.field.stopcounterovfl;
	args->haltt = r0.field.stoptimerovfl;

	r1.val = DLB2_CSR_RD(hw, addr->cfg1);

	args->sel0 = r1.field.mode0;
	args->sel1 = r1.field.mode1;

	args->cmpv0 = DLB2_CSR_RD(hw, addr->compare0);

	args->cmpv1 = DLB2_CSR_RD(hw, addr->compare1);

	if (args->smon_id == DLB2_SMON_SYS0 ||
	    args->smon_id == DLB2_SMON_SYS1 ||
	    args->smon_id == DLB2_SMON_IOSF0 ||
	    args->smon_id == DLB2_SMON_IOSF1 ||
	    args->smon_id == DLB2_SMON_PM) {
		args->mask0 = DLB2_CSR_RD(hw, addr->comp_mask0);

		args->mask1 = DLB2_CSR_RD(hw, addr->comp_mask1);
	}

	args->cnt0 = DLB2_CSR_RD(hw, addr->activitycntr0);

	args->cnt1 = DLB2_CSR_RD(hw, addr->activitycntr1);

	args->timer = DLB2_CSR_RD(hw, addr->tmr);

	args->max_timer = DLB2_CSR_RD(hw, addr->max_tmr);

	if (args->smon_id == DLB2_SMON_LSP) {
		union dlb2_lsp_cfg_ctrl_general_0 r2;

		r2.val = DLB2_CSR_RD(hw, DLB2_LSP_CFG_CTRL_GENERAL_0);

		args->lsp_valid_sel = r2.field.smon0_valid_sel;
		args->lsp_value_sel = r2.field.smon0_value_sel;
		args->lsp_compare_sel = r2.field.smon0_compare_sel;
	}

	return 0;
}

static void dlb2_hw_send_async_pf_to_vf_msg(struct dlb2_hw *hw,
					    unsigned int vf_id)
{
	union dlb2_func_pf_pf2vf_mailbox_isr r0 = { {0} };

	switch (vf_id) {
	case 0:
			r0.field.vf0_isr = 1;
		break;
	case 1:
		r0.field.vf1_isr = 1;
		break;
	case 2:
		r0.field.vf2_isr = 1;
		break;
	case 3:
		r0.field.vf3_isr = 1;
		break;
	case 4:
		r0.field.vf4_isr = 1;
		break;
	case 5:
		r0.field.vf5_isr = 1;
		break;
	case 6:
		r0.field.vf6_isr = 1;
		break;
	case 7:
		r0.field.vf7_isr = 1;
		break;
	case 8:
		r0.field.vf8_isr = 1;
		break;
	case 9:
		r0.field.vf9_isr = 1;
		break;
	case 10:
		r0.field.vf10_isr = 1;
		break;
	case 11:
		r0.field.vf11_isr = 1;
		break;
	case 12:
		r0.field.vf12_isr = 1;
		break;
	case 13:
		r0.field.vf13_isr = 1;
		break;
	case 14:
		r0.field.vf14_isr = 1;
		break;
	case 15:
		r0.field.vf15_isr = 1;
		break;
	default:
		break;
	}

	DLB2_FUNC_WR(hw, DLB2_FUNC_PF_PF2VF_MAILBOX_ISR(0), r0.val);
}

static void dlb2_sw_send_async_pf_to_vdev_msg(struct dlb2_hw *hw,
					      unsigned int vdev_id)
{
	void *arg = hw->mbox[vdev_id].pf_to_vdev_inject_arg;

	/* Set the ISR in progress bit. The vdev driver will clear it. */
	*hw->mbox[vdev_id].pf_to_vdev.isr_in_progress = 1;

	hw->mbox[vdev_id].pf_to_vdev_inject(arg);
}

void dlb2_send_async_pf_to_vdev_msg(struct dlb2_hw *hw, unsigned int vdev_id)
{
	if (hw->virt_mode == DLB2_VIRT_SIOV)
		dlb2_sw_send_async_pf_to_vdev_msg(hw, vdev_id);
	else
		dlb2_hw_send_async_pf_to_vf_msg(hw, vdev_id);
}

static bool dlb2_hw_pf_to_vf_complete(struct dlb2_hw *hw, unsigned int vf_id)
{
	union dlb2_func_pf_pf2vf_mailbox_isr r0;

	r0.val = DLB2_FUNC_RD(hw, DLB2_FUNC_PF_PF2VF_MAILBOX_ISR(vf_id));

	return (r0.val & (1 << vf_id)) == 0;
}

static bool dlb2_sw_pf_to_vdev_complete(struct dlb2_hw *hw,
					unsigned int vdev_id)
{
	return !(*hw->mbox[vdev_id].pf_to_vdev.isr_in_progress);
}

bool dlb2_pf_to_vdev_complete(struct dlb2_hw *hw, unsigned int vdev_id)
{
	if (hw->virt_mode == DLB2_VIRT_SIOV)
		return dlb2_sw_pf_to_vdev_complete(hw, vdev_id);
	else
		return dlb2_hw_pf_to_vf_complete(hw, vdev_id);
}

void dlb2_send_async_vdev_to_pf_msg(struct dlb2_hw *hw)
{
	union dlb2_func_vf_vf2pf_mailbox_isr r0 = { {0} };
	u32 offs;

	if (hw->virt_mode == DLB2_VIRT_SIOV)
		offs = DLB2_FUNC_VF_SIOV_VF2PF_MAILBOX_ISR_TRIGGER;
	else
		offs = DLB2_FUNC_VF_VF2PF_MAILBOX_ISR;

	r0.field.isr = 1;

	DLB2_FUNC_WR(hw, offs, r0.val);
}

bool dlb2_vdev_to_pf_complete(struct dlb2_hw *hw)
{
	union dlb2_func_vf_vf2pf_mailbox_isr r0;

	r0.val = DLB2_FUNC_RD(hw, DLB2_FUNC_VF_VF2PF_MAILBOX_ISR);

	return (r0.field.isr == 0);
}

bool dlb2_vf_flr_complete(struct dlb2_hw *hw)
{
	union dlb2_func_vf_vf_reset_in_progress r0;

	r0.val = DLB2_FUNC_RD(hw, DLB2_FUNC_VF_VF_RESET_IN_PROGRESS);

	return (r0.field.reset_in_progress == 0);
}

static u32 dlb2_read_vf2pf_mbox(struct dlb2_hw *hw,
				unsigned int id,
				int offs,
				bool req)
{
	u32 idx = offs;

	if (req)
		idx += DLB2_VF2PF_REQ_BASE_WORD;
	else
		idx += DLB2_VF2PF_RESP_BASE_WORD;

	if (hw->virt_mode == DLB2_VIRT_SIOV)
		return hw->mbox[id].vdev_to_pf.mbox[offs];
	else
		return DLB2_FUNC_RD(hw, DLB2_FUNC_PF_VF2PF_MAILBOX(id, idx));
}

int dlb2_pf_read_vf_mbox_req(struct dlb2_hw *hw,
			     unsigned int vdev_id,
			     void *data,
			     int len)
{
	u32 buf[DLB2_VF2PF_REQ_BYTES / 4];
	int num_words;
	int i;

	if (len > DLB2_VF2PF_REQ_BYTES) {
		DLB2_BASE_ERR(hw,
			      "[%s()] len (%d) > VF->PF mailbox req size\n",
			      __func__, len);
		return -EINVAL;
	}

	if (len == 0) {
		DLB2_BASE_ERR(hw, "[%s()] invalid len (0)\n", __func__);
		return -EINVAL;
	}

	if (hw->virt_mode == DLB2_VIRT_SIOV &&
	    !hw->mbox[vdev_id].vdev_to_pf.mbox) {
		DLB2_BASE_ERR(hw, "[%s()] No mailbox registered for vdev %d\n",
			      __func__, vdev_id);
		return -EINVAL;
	}

	/* Round up len to the nearest 4B boundary, since the mailbox registers
	 * are 32b wide.
	 */
	num_words = len / 4;
	if (len % 4 != 0)
		num_words++;

	for (i = 0; i < num_words; i++)
		buf[i] = dlb2_read_vf2pf_mbox(hw, vdev_id, i, true);

	memcpy(data, buf, len);

	return 0;
}

int dlb2_pf_read_vf_mbox_resp(struct dlb2_hw *hw,
			      unsigned int vdev_id,
			      void *data,
			      int len)
{
	u32 buf[DLB2_VF2PF_RESP_BYTES / 4];
	int num_words;
	int i;

	if (len > DLB2_VF2PF_RESP_BYTES) {
		DLB2_BASE_ERR(hw,
			      "[%s()] len (%d) > VF->PF mailbox resp size\n",
			      __func__, len);
		return -EINVAL;
	}

	/* Round up len to the nearest 4B boundary, since the mailbox registers
	 * are 32b wide.
	 */
	num_words = len / 4;
	if (len % 4 != 0)
		num_words++;

	for (i = 0; i < num_words; i++)
		buf[i] = dlb2_read_vf2pf_mbox(hw, vdev_id, i, false);

	memcpy(data, buf, len);

	return 0;
}

static void dlb2_write_pf2vf_mbox_resp(struct dlb2_hw *hw,
				       unsigned int vdev_id,
				       int offs,
				       u32 data)
{
	u32 idx = offs + DLB2_PF2VF_RESP_BASE_WORD;

	if (hw->virt_mode == DLB2_VIRT_SIOV)
		hw->mbox[vdev_id].pf_to_vdev.mbox[idx] = data;
	else
		DLB2_FUNC_WR(hw,
			     DLB2_FUNC_PF_PF2VF_MAILBOX(vdev_id, idx),
			     data);
}

int dlb2_pf_write_vf_mbox_resp(struct dlb2_hw *hw,
			       unsigned int vdev_id,
			       void *data,
			       int len)
{
	u32 buf[DLB2_PF2VF_RESP_BYTES / 4];
	int num_words;
	int i;

	if (len > DLB2_PF2VF_RESP_BYTES) {
		DLB2_BASE_ERR(hw,
			      "[%s()] len (%d) > PF->VF mailbox resp size\n",
			      __func__, len);
		return -EINVAL;
	}

	if (hw->virt_mode == DLB2_VIRT_SIOV &&
	    !hw->mbox[vdev_id].pf_to_vdev.mbox) {
		DLB2_BASE_ERR(hw, "[%s()] No mailbox registered for vdev %d\n",
			      __func__, vdev_id);
		return -EINVAL;
	}

	memcpy(buf, data, len);

	/* Round up len to the nearest 4B boundary, since the mailbox registers
	 * are 32b wide.
	 */
	num_words = len / 4;
	if (len % 4 != 0)
		num_words++;

	for (i = 0; i < num_words; i++)
		dlb2_write_pf2vf_mbox_resp(hw, vdev_id, i, buf[i]);

	return 0;
}

static void dlb2_write_pf2vf_mbox_req(struct dlb2_hw *hw,
				      unsigned int vdev_id,
				      int offs,
				      u32 data)
{
	u32 idx = offs + DLB2_PF2VF_REQ_BASE_WORD;

	if (hw->virt_mode == DLB2_VIRT_SIOV)
		hw->mbox[vdev_id].pf_to_vdev.mbox[idx] = data;
	else
		DLB2_FUNC_WR(hw,
			     DLB2_FUNC_PF_PF2VF_MAILBOX(vdev_id, idx),
			     data);
}

int dlb2_pf_write_vf_mbox_req(struct dlb2_hw *hw,
			      unsigned int vdev_id,
			      void *data,
			      int len)
{
	u32 buf[DLB2_PF2VF_REQ_BYTES / 4];
	int num_words;
	int i;

	if (len > DLB2_PF2VF_REQ_BYTES) {
		DLB2_BASE_ERR(hw,
			      "[%s()] len (%d) > PF->VF mailbox req size\n",
			      __func__, len);
		return -EINVAL;
	}

	memcpy(buf, data, len);

	/* Round up len to the nearest 4B boundary, since the mailbox registers
	 * are 32b wide.
	 */
	num_words = len / 4;
	if (len % 4 != 0)
		num_words++;

	for (i = 0; i < num_words; i++)
		dlb2_write_pf2vf_mbox_req(hw, vdev_id, i, buf[i]);

	return 0;
}

int dlb2_vf_read_pf_mbox_resp(struct dlb2_hw *hw, void *data, int len)
{
	u32 buf[DLB2_PF2VF_RESP_BYTES / 4];
	int num_words;
	int i;

	if (len > DLB2_PF2VF_RESP_BYTES) {
		DLB2_BASE_ERR(hw,
			      "[%s()] len (%d) > PF->VF mailbox resp size\n",
			      __func__, len);
		return -EINVAL;
	}

	if (len == 0) {
		DLB2_BASE_ERR(hw, "[%s()] invalid len (0)\n", __func__);
		return -EINVAL;
	}

	/* Round up len to the nearest 4B boundary, since the mailbox registers
	 * are 32b wide.
	 */
	num_words = len / 4;
	if (len % 4 != 0)
		num_words++;

	for (i = 0; i < num_words; i++) {
		u32 idx = i + DLB2_PF2VF_RESP_BASE_WORD;

		buf[i] = DLB2_FUNC_RD(hw, DLB2_FUNC_VF_PF2VF_MAILBOX(idx));
	}

	memcpy(data, buf, len);

	return 0;
}

int dlb2_vf_read_pf_mbox_req(struct dlb2_hw *hw, void *data, int len)
{
	u32 buf[DLB2_PF2VF_REQ_BYTES / 4];
	int num_words;
	int i;

	if (len > DLB2_PF2VF_REQ_BYTES) {
		DLB2_BASE_ERR(hw,
			      "[%s()] len (%d) > PF->VF mailbox req size\n",
			      __func__, len);
		return -EINVAL;
	}

	/* Round up len to the nearest 4B boundary, since the mailbox registers
	 * are 32b wide.
	 */
	num_words = len / 4;
	if ((len % 4) != 0)
		num_words++;

	for (i = 0; i < num_words; i++) {
		u32 idx = i + DLB2_PF2VF_REQ_BASE_WORD;

		buf[i] = DLB2_FUNC_RD(hw, DLB2_FUNC_VF_PF2VF_MAILBOX(idx));
	}

	memcpy(data, buf, len);

	return 0;
}

int dlb2_vf_write_pf_mbox_req(struct dlb2_hw *hw, void *data, int len)
{
	u32 buf[DLB2_VF2PF_REQ_BYTES / 4];
	int num_words;
	int i;

	if (len > DLB2_VF2PF_REQ_BYTES) {
		DLB2_BASE_ERR(hw,
			      "[%s()] len (%d) > VF->PF mailbox req size\n",
			      __func__, len);
		return -EINVAL;
	}

	memcpy(buf, data, len);

	/* Round up len to the nearest 4B boundary, since the mailbox registers
	 * are 32b wide.
	 */
	num_words = len / 4;
	if (len % 4 != 0)
		num_words++;

	for (i = 0; i < num_words; i++) {
		u32 idx = i + DLB2_VF2PF_REQ_BASE_WORD;

		DLB2_FUNC_WR(hw, DLB2_FUNC_VF_VF2PF_MAILBOX(idx), buf[i]);
	}

	return 0;
}

int dlb2_vf_write_pf_mbox_resp(struct dlb2_hw *hw, void *data, int len)
{
	u32 buf[DLB2_VF2PF_RESP_BYTES / 4];
	int num_words;
	int i;

	if (len > DLB2_VF2PF_RESP_BYTES) {
		DLB2_BASE_ERR(hw,
			      "[%s()] len (%d) > VF->PF mailbox resp size\n",
			      __func__, len);
		return -EINVAL;
	}

	memcpy(buf, data, len);

	/* Round up len to the nearest 4B boundary, since the mailbox registers
	 * are 32b wide.
	 */
	num_words = len / 4;
	if (len % 4 != 0)
		num_words++;

	for (i = 0; i < num_words; i++) {
		u32 idx = i + DLB2_VF2PF_RESP_BASE_WORD;

		DLB2_FUNC_WR(hw, DLB2_FUNC_VF_VF2PF_MAILBOX(idx), buf[i]);
	}

	return 0;
}

bool dlb2_vdev_is_locked(struct dlb2_hw *hw, unsigned int id)
{
	return hw->vdev[id].locked;
}

static void dlb2_vf_set_rsrc_virt_ids(struct dlb2_function_resources *rsrcs,
				      unsigned int id)
{
	struct dlb2_list_entry *iter __attribute__((unused));
	struct dlb2_dir_pq_pair *dir_port;
	struct dlb2_ldb_queue *ldb_queue;
	struct dlb2_ldb_port *ldb_port;
	struct dlb2_domain *domain;
	int i, j;

	i = 0;
	DLB2_FUNC_LIST_FOR(rsrcs->avail_domains, domain, iter) {
		domain->id.virt_id = i;
		domain->id.vdev_owned = true;
		domain->id.vdev_id = id;
		i++;
	}

	i = 0;
	DLB2_FUNC_LIST_FOR(rsrcs->avail_ldb_queues, ldb_queue, iter) {
		ldb_queue->id.virt_id = i;
		ldb_queue->id.vdev_owned = true;
		ldb_queue->id.vdev_id = id;
		i++;
	}

	i = 0;
	for (j = 0; j < DLB2_NUM_COS_DOMAINS; j++) {
		DLB2_FUNC_LIST_FOR(rsrcs->avail_ldb_ports[j], ldb_port, iter) {
			ldb_port->id.virt_id = i;
			ldb_port->id.vdev_owned = true;
			ldb_port->id.vdev_id = id;
			i++;
		}
	}

	i = 0;
	DLB2_FUNC_LIST_FOR(rsrcs->avail_dir_pq_pairs, dir_port, iter) {
		dir_port->id.virt_id = i;
		dir_port->id.vdev_owned = true;
		dir_port->id.vdev_id = id;
		i++;
	}
}

void dlb2_lock_vdev(struct dlb2_hw *hw, unsigned int id)
{
	struct dlb2_function_resources *rsrcs = &hw->vdev[id];

	rsrcs->locked = true;

	dlb2_vf_set_rsrc_virt_ids(rsrcs, id);
}

void dlb2_unlock_vdev(struct dlb2_hw *hw, unsigned int id)
{
	hw->vdev[id].locked = false;
}

int dlb2_reset_vdev_resources(struct dlb2_hw *hw, unsigned int id)
{
	if (id >= DLB2_MAX_NUM_VDEVS)
		return -EINVAL;

	/* If the VF is locked, its resource assignment can't be changed */
	if (dlb2_vdev_is_locked(hw, id))
		return -EPERM;

	dlb2_update_vdev_sched_domains(hw, id, 0);
	dlb2_update_vdev_ldb_queues(hw, id, 0);
	dlb2_update_vdev_ldb_ports(hw, id, 0);
	dlb2_update_vdev_dir_ports(hw, id, 0);
	dlb2_update_vdev_ldb_credits(hw, id, 0);
	dlb2_update_vdev_dir_credits(hw, id, 0);
	dlb2_update_vdev_hist_list_entries(hw, id, 0);
	dlb2_update_vdev_atomic_inflights(hw, id, 0);

	return 0;
}

void dlb2_clr_pmcsr_disable(struct dlb2_hw *hw)
{
	union dlb2_cfg_mstr_cfg_pm_pmcsr_disable r0;

	r0.val = DLB2_CSR_RD(hw, DLB2_CFG_MSTR_CFG_PM_PMCSR_DISABLE);

	r0.field.disable = 0;

	DLB2_CSR_WR(hw, DLB2_CFG_MSTR_CFG_PM_PMCSR_DISABLE, r0.val);
}

int dlb2_hw_set_virt_mode(struct dlb2_hw *hw, enum dlb2_virt_mode mode)
{
	if (mode >= NUM_DLB2_VIRT_MODES)
		return -EINVAL;

	hw->virt_mode = mode;

	return 0;
}

enum dlb2_virt_mode dlb2_hw_get_virt_mode(struct dlb2_hw *hw)
{
	return hw->virt_mode;
}

s32 dlb2_hw_get_ldb_port_phys_id(struct dlb2_hw *hw,
				 u32 id,
				 unsigned int vdev_id)
{
	struct dlb2_ldb_port *port;

	port = dlb2_get_ldb_port_from_id(hw, id, true, vdev_id);
	if (!port)
		return -1;

	return port->id.phys_id;
}

s32 dlb2_hw_get_dir_port_phys_id(struct dlb2_hw *hw,
				 u32 id,
				 unsigned int vdev_id)
{
	struct dlb2_dir_pq_pair *port;

	port = dlb2_get_dir_pq_from_id(hw, id, true, vdev_id);
	if (!port)
		return -1;

	return port->id.phys_id;
}

void dlb2_hw_register_sw_mbox(struct dlb2_hw *hw,
			      unsigned int vdev_id,
			      u32 *vdev_to_pf_mbox,
			      u32 *pf_to_vdev_mbox,
			      void (*inject)(void *),
			      void *inject_arg)
{
	u32 offs;

	offs = DLB2_FUNC_VF_VF2PF_MAILBOX_ISR % 0x1000;

	hw->mbox[vdev_id].vdev_to_pf.mbox = vdev_to_pf_mbox;
	hw->mbox[vdev_id].vdev_to_pf.isr_in_progress =
		(u32 *)((u8 *)vdev_to_pf_mbox + offs);

	offs = (DLB2_FUNC_VF_PF2VF_MAILBOX_ISR % 0x1000);

	hw->mbox[vdev_id].pf_to_vdev.mbox = pf_to_vdev_mbox;
	hw->mbox[vdev_id].pf_to_vdev.isr_in_progress =
		(u32 *)((u8 *)pf_to_vdev_mbox + offs);

	hw->mbox[vdev_id].pf_to_vdev_inject = inject;
	hw->mbox[vdev_id].pf_to_vdev_inject_arg = inject_arg;
}

void dlb2_hw_unregister_sw_mbox(struct dlb2_hw *hw, unsigned int vdev_id)
{
	hw->mbox[vdev_id].vdev_to_pf.mbox = NULL;
	hw->mbox[vdev_id].pf_to_vdev.mbox = NULL;
	hw->mbox[vdev_id].vdev_to_pf.isr_in_progress = NULL;
	hw->mbox[vdev_id].pf_to_vdev.isr_in_progress = NULL;
	hw->mbox[vdev_id].pf_to_vdev_inject = NULL;
	hw->mbox[vdev_id].pf_to_vdev_inject_arg = NULL;
}

void dlb2_hw_setup_cq_ims_entry(struct dlb2_hw *hw,
				unsigned int vdev_id,
				u32 virt_cq_id,
				bool is_ldb,
				u32 addr_lo,
				u32 data)
{
	s32 cq_id;

	if (hw->virt_mode != DLB2_VIRT_SIOV) {
		DLB2_BASE_ERR(hw,
			      "[%s()] Internal error: incorrect virt mode\n",
			      __func__);
		return;
	}

	if (is_ldb) {
		union dlb2_sys_ldb_cq_ai_addr r0 = { {0} };
		union dlb2_sys_ldb_cq_ai_data r1 = { {0} };

		cq_id = dlb2_hw_get_ldb_port_phys_id(hw, virt_cq_id, vdev_id);
		if (cq_id < 0) {
			DLB2_BASE_ERR(hw, "[%s()] Failed to lookup CQ ID\n",
				      __func__);
			return;
		}

		r0.val = addr_lo;
		r1.val = data;

		DLB2_CSR_WR(hw, DLB2_SYS_LDB_CQ_AI_ADDR(cq_id), r0.val);
		DLB2_CSR_WR(hw, DLB2_SYS_LDB_CQ_AI_DATA(cq_id), r1.val);
	} else {
		union dlb2_sys_dir_cq_ai_addr r0 = { {0} };
		union dlb2_sys_dir_cq_ai_data r1 = { {0} };

		cq_id = dlb2_hw_get_dir_port_phys_id(hw, virt_cq_id, vdev_id);
		if (cq_id < 0) {
			DLB2_BASE_ERR(hw, "[%s()] Failed to lookup CQ ID\n",
				      __func__);
			return;
		}

		r0.val = addr_lo;
		r1.val = data;

		DLB2_CSR_WR(hw, DLB2_SYS_DIR_CQ_AI_ADDR(cq_id), r0.val);
		DLB2_CSR_WR(hw, DLB2_SYS_DIR_CQ_AI_DATA(cq_id), r1.val);
	}

	dlb2_flush_csr(hw);
}

void dlb2_hw_clear_cq_ims_entry(struct dlb2_hw *hw,
				unsigned int vdev_id,
				u32 virt_cq_id,
				bool is_ldb)
{
	dlb2_hw_setup_cq_ims_entry(hw, vdev_id, virt_cq_id, is_ldb, 0, 0);
}

int dlb2_hw_register_pasid(struct dlb2_hw *hw,
			   unsigned int vdev_id,
			   unsigned int pasid)
{
	if (vdev_id >= DLB2_MAX_NUM_VDEVS)
		return -1;

	hw->pasid[vdev_id] = pasid;

	return 0;
}

int dlb2_hw_get_cos_bandwidth(struct dlb2_hw *hw, u32 cos_id)
{
	if (cos_id >= DLB2_NUM_COS_DOMAINS)
		return -EINVAL;

	return hw->cos_reservation[cos_id];
}

static void dlb2_log_set_cos_bandwidth(struct dlb2_hw *hw, u32 cos_id, u8 bw)
{
	DLB2_BASE_DBG(hw, "DLB2 set port CoS bandwidth:\n");
	DLB2_BASE_DBG(hw, "\tCoS ID:    %u\n", cos_id);
	DLB2_BASE_DBG(hw, "\tBandwidth: %u\n", bw);
}

int dlb2_hw_set_cos_bandwidth(struct dlb2_hw *hw, u32 cos_id, u8 bandwidth)
{
	union dlb2_lsp_cfg_shdw_range_cos r0 = { {0} };
	union dlb2_lsp_cfg_shdw_ctrl r1 = { {0} };
	unsigned int i;
	u8 total;

	if (cos_id >= DLB2_NUM_COS_DOMAINS)
		return -EINVAL;

	if (bandwidth > 100)
		return -EINVAL;

	total = 0;

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++)
		total += (i == cos_id) ? bandwidth : hw->cos_reservation[i];

	if (total > 100)
		return -EINVAL;

	r0.val = DLB2_CSR_RD(hw, DLB2_LSP_CFG_SHDW_RANGE_COS(cos_id));

	/* Normalize the bandwidth to a value in the range 0-255. Integer
	 * division may leave unreserved scheduling slots; these will be
	 * divided among the 4 classes of service.
	 */
	r0.field.bw_range = (bandwidth * 256) / 100;

	DLB2_CSR_WR(hw, DLB2_LSP_CFG_SHDW_RANGE_COS(cos_id), r0.val);

	r1.field.transfer = 1;

	/* Atomically transfer the newly configured service weight */
	DLB2_CSR_WR(hw, DLB2_LSP_CFG_SHDW_CTRL, r1.val);

	dlb2_log_set_cos_bandwidth(hw, cos_id, bandwidth);

	hw->cos_reservation[cos_id] = bandwidth;

	return 0;
}

struct dlb2_wd_config {
	u32 threshold;
	u32 interval;
};

int dlb2_hw_enable_wd_timer(struct dlb2_hw *hw, enum dlb2_wd_tmo tmo)
{
	/* Timeout = num_ports * threshold * (sample interval + 1) / 100 MHz */
	const struct dlb2_wd_config wd_config[NUM_DLB2_WD_TMOS] = {
		[DLB2_WD_TMO_40S] = {30, 0x1FFFFF},
		[DLB2_WD_TMO_10S] = {30, 0x7FFFF},
		[DLB2_WD_TMO_1S]  = {24, 0xFFFF},
	};
	union dlb2_chp_cfg_dir_wd_threshold r0 = { {0} };
	union dlb2_chp_cfg_ldb_wd_threshold r1 = { {0} };
	union dlb2_chp_cfg_dir_wd_enb_interval r2 = { {0} };
	union dlb2_chp_cfg_ldb_wd_enb_interval r3 = { {0} };

	if (tmo > NUM_DLB2_WD_TMOS)
		return -EINVAL;

	r0.field.wd_threshold = wd_config[tmo].threshold;
	r1.field.wd_threshold = wd_config[tmo].threshold;

	DLB2_CSR_WR(hw, DLB2_CHP_CFG_DIR_WD_THRESHOLD, r0.val);
	DLB2_CSR_WR(hw, DLB2_CHP_CFG_LDB_WD_THRESHOLD, r1.val);

	r2.field.sample_interval = wd_config[tmo].interval;
	r3.field.sample_interval = wd_config[tmo].interval;
	r2.field.enb = 1;
	r3.field.enb = 1;

	/* If running on the emulation platform, adjust accordingly */
	if (DLB2_HZ == 2000000) {
		r2.field.sample_interval /= 400;
		r3.field.sample_interval /= 400;
	}

	DLB2_CSR_WR(hw, DLB2_CHP_CFG_DIR_WD_ENB_INTERVAL, r2.val);
	DLB2_CSR_WR(hw, DLB2_CHP_CFG_LDB_WD_ENB_INTERVAL, r3.val);

	return 0;
}

int dlb2_hw_enable_dir_cq_wd_int(struct dlb2_hw *hw,
				 u32 id,
				 bool vdev_req,
				 unsigned int vdev_id)
{
	union dlb2_chp_dir_cq_wd_enb r0 = { {0} };
	union dlb2_chp_cfg_dir_wd_disable0 r1 = { {0} };
	struct dlb2_dir_pq_pair *port;

	port = dlb2_get_dir_pq_from_id(hw, id, vdev_req, vdev_id);
	if (!port)
		return -EINVAL;

	r0.field.wd_enable = 1;

	DLB2_CSR_WR(hw, DLB2_CHP_DIR_CQ_WD_ENB(port->id.phys_id), r0.val);

	r1.val |= 1 << (port->id.phys_id % 32);

	/* WD_DISABLE registers are W1CLR */
	if (port->id.phys_id < 32)
		DLB2_CSR_WR(hw, DLB2_CHP_CFG_DIR_WD_DISABLE0, r1.val);
	else
		DLB2_CSR_WR(hw, DLB2_CHP_CFG_DIR_WD_DISABLE1, r1.val);

	return 0;
}

int dlb2_hw_enable_ldb_cq_wd_int(struct dlb2_hw *hw,
				 u32 id,
				 bool vdev_req,
				 unsigned int vdev_id)
{
	union dlb2_chp_ldb_cq_wd_enb r0 = { {0} };
	union dlb2_chp_cfg_ldb_wd_disable0 r1 = { {0} };
	struct dlb2_ldb_port *port;

	port = dlb2_get_ldb_port_from_id(hw, id, vdev_req, vdev_id);
	if (!port)
		return -EINVAL;

	r0.field.wd_enable = 1;

	DLB2_CSR_WR(hw, DLB2_CHP_LDB_CQ_WD_ENB(port->id.phys_id), r0.val);

	r1.val |= 1 << (port->id.phys_id % 32);

	/* WD_DISABLE registers are W1CLR */
	if (port->id.phys_id < 32)
		DLB2_CSR_WR(hw, DLB2_CHP_CFG_LDB_WD_DISABLE0, r1.val);
	else
		DLB2_CSR_WR(hw, DLB2_CHP_CFG_LDB_WD_DISABLE1, r1.val);

	return 0;
}

void dlb2_hw_enable_sparse_ldb_cq_mode(struct dlb2_hw *hw)
{
	union dlb2_chp_cfg_chp_csr_ctrl r0;

	r0.val = DLB2_CSR_RD(hw, DLB2_CHP_CFG_CHP_CSR_CTRL);

	r0.field.cfg_64bytes_qe_ldb_cq_mode = 1;

	DLB2_CSR_WR(hw, DLB2_CHP_CFG_CHP_CSR_CTRL, r0.val);
}

void dlb2_hw_enable_sparse_dir_cq_mode(struct dlb2_hw *hw)
{
	union dlb2_chp_cfg_chp_csr_ctrl r0;

	r0.val = DLB2_CSR_RD(hw, DLB2_CHP_CFG_CHP_CSR_CTRL);

	r0.field.cfg_64bytes_qe_dir_cq_mode = 1;

	DLB2_CSR_WR(hw, DLB2_CHP_CFG_CHP_CSR_CTRL, r0.val);
}

void dlb2_hw_disable_sparse_ldb_cq_mode(struct dlb2_hw *hw)
{
	union dlb2_chp_cfg_chp_csr_ctrl r0;

	r0.val = DLB2_CSR_RD(hw, DLB2_CHP_CFG_CHP_CSR_CTRL);

	r0.field.cfg_64bytes_qe_ldb_cq_mode = 0;

	DLB2_CSR_WR(hw, DLB2_CHP_CFG_CHP_CSR_CTRL, r0.val);
}

void dlb2_hw_disable_sparse_dir_cq_mode(struct dlb2_hw *hw)
{
	union dlb2_chp_cfg_chp_csr_ctrl r0;

	r0.val = DLB2_CSR_RD(hw, DLB2_CHP_CFG_CHP_CSR_CTRL);

	r0.field.cfg_64bytes_qe_dir_cq_mode = 0;

	DLB2_CSR_WR(hw, DLB2_CHP_CFG_CHP_CSR_CTRL, r0.val);
}

void dlb2_hw_set_qe_arbiter_weights(struct dlb2_hw *hw, u8 weight[8])
{
	union dlb2_atm_cfg_arb_weights_rdy_bin r0 = { {0} };
	union dlb2_nalb_pipe_cfg_arb_weights_tqpri_nalb_0 r1 = { {0} };
	union dlb2_nalb_pipe_cfg_arb_weights_tqpri_replay_0 r2 = { {0} };
	union dlb2_dp_cfg_arb_weights_tqpri_replay_0 r3 = { {0} };
	union dlb2_dp_cfg_arb_weights_tqpri_dir_0 r4 =  { {0} };
	union dlb2_nalb_pipe_cfg_arb_weights_tqpri_atq_0 r5 = { {0} };
	union dlb2_atm_cfg_arb_weights_sched_bin r6 = { {0} };
	union dlb2_aqed_pipe_cfg_arb_weights_tqpri_atm_0 r7 = { {0} };

	r0.field.bin0 = weight[1];
	r0.field.bin1 = weight[3];
	r0.field.bin2 = weight[5];
	r0.field.bin3 = weight[7];

	r1.field.pri0 = weight[1];
	r1.field.pri1 = weight[3];
	r1.field.pri2 = weight[5];
	r1.field.pri3 = weight[7];

	r2.field.pri0 = weight[1];
	r2.field.pri1 = weight[3];
	r2.field.pri2 = weight[5];
	r2.field.pri3 = weight[7];

	r3.field.pri0 = weight[1];
	r3.field.pri1 = weight[3];
	r3.field.pri2 = weight[5];
	r3.field.pri3 = weight[7];

	r4.field.pri0 = weight[1];
	r4.field.pri1 = weight[3];
	r4.field.pri2 = weight[5];
	r4.field.pri3 = weight[7];

	r5.field.pri0 = weight[1];
	r5.field.pri1 = weight[3];
	r5.field.pri2 = weight[5];
	r5.field.pri3 = weight[7];

	r6.field.bin0 = weight[1];
	r6.field.bin1 = weight[3];
	r6.field.bin2 = weight[5];
	r6.field.bin3 = weight[7];

	r7.field.pri0 = weight[1];
	r7.field.pri1 = weight[3];
	r7.field.pri2 = weight[5];
	r7.field.pri3 = weight[7];

	DLB2_CSR_WR(hw, DLB2_ATM_CFG_ARB_WEIGHTS_RDY_BIN, r0.val);
	DLB2_CSR_WR(hw, DLB2_NALB_PIPE_CFG_ARB_WEIGHTS_TQPRI_NALB_0, r1.val);
	DLB2_CSR_WR(hw, DLB2_NALB_PIPE_CFG_ARB_WEIGHTS_TQPRI_REPLAY_0, r2.val);
	DLB2_CSR_WR(hw, DLB2_DP_CFG_ARB_WEIGHTS_TQPRI_REPLAY_0, r3.val);
	DLB2_CSR_WR(hw, DLB2_DP_CFG_ARB_WEIGHTS_TQPRI_DIR_0, r4.val);
	DLB2_CSR_WR(hw, DLB2_NALB_PIPE_CFG_ARB_WEIGHTS_TQPRI_ATQ_0, r5.val);
	DLB2_CSR_WR(hw, DLB2_ATM_CFG_ARB_WEIGHTS_SCHED_BIN, r6.val);
	DLB2_CSR_WR(hw, DLB2_AQED_PIPE_CFG_ARB_WEIGHTS_TQPRI_ATM_0, r7.val);
}

void dlb2_hw_set_qid_arbiter_weights(struct dlb2_hw *hw, u8 weight[8])
{
	union dlb2_lsp_cfg_arb_weight_ldb_qid_0 r0 = { {0} };
	union dlb2_lsp_cfg_arb_weight_atm_nalb_qid_0 r1 = { {0} };

	r0.field.pri0_weight = weight[0];
	r0.field.pri1_weight = weight[3];
	r0.field.pri2_weight = weight[5];
	r0.field.pri3_weight = weight[7];

	r1.field.pri0_weight = weight[1];
	r1.field.pri1_weight = weight[3];
	r1.field.pri2_weight = weight[5];
	r1.field.pri3_weight = weight[7];

	DLB2_CSR_WR(hw, DLB2_LSP_CFG_ARB_WEIGHT_LDB_QID_0, r0.val);
	DLB2_CSR_WR(hw, DLB2_LSP_CFG_ARB_WEIGHT_ATM_NALB_QID_0, r1.val);
}
