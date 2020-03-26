/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
 * Copyright(c) 2016-2018 Intel Corporation
 */

#ifndef __HQMV2_HW_TYPES_H
#define __HQMV2_HW_TYPES_H

#include "uapi/linux/hqmv2_user.h"
#include "hqmv2_osdep_types.h"
#include "hqmv2_osdep_list.h"

#define HQMV2_MAX_NUM_VDEVS 16 //May change to 4 in the future
#define HQMV2_MAX_NUM_DOMAINS 32
#define HQMV2_MAX_NUM_LDB_QUEUES 32
#define HQMV2_MAX_NUM_DIR_QUEUES 64
#define HQMV2_MAX_NUM_LDB_PORTS 64
#define HQMV2_MAX_NUM_DIR_PORTS HQMV2_MAX_NUM_DIR_QUEUES
#define HQMV2_MAX_NUM_LDB_CREDITS 8192
#define HQMV2_MAX_NUM_DIR_CREDITS 2048
#define HQMV2_MAX_NUM_HIST_LIST_ENTRIES 2048
#define HQMV2_MAX_NUM_AQED_ENTRIES 2048
#define HQMV2_MAX_NUM_QIDS_PER_LDB_CQ 8
#define HQMV2_MAX_NUM_SEQUENCE_NUMBER_GROUPS 2
#define HQMV2_MAX_NUM_SEQUENCE_NUMBER_MODES 5
#define HQMV2_QID_PRIORITIES 8
#define HQMV2_NUM_COS_DOMAINS 4
#define HQMV2_MAX_CQ_COMP_CHECK_LOOPS 409600
#define HQMV2_MAX_QID_EMPTY_CHECK_LOOPS (32 * 64 * 1024 * (800 / 30))
/* FIXME: 2 MHz FPGA frequency. Change when ASIC available. */
#define HQMV2_HZ 2000000

#define HQMV2_SPR_PF_DEV_ID 0x2710
#define HQMV2_SPR_VF_DEV_ID 0x2711

/* Interrupt related macros */
#define HQMV2_PF_NUM_NON_CQ_INTERRUPT_VECTORS 1
#define HQMV2_PF_NUM_CQ_INTERRUPT_VECTORS	 64
#define HQMV2_PF_TOTAL_NUM_INTERRUPT_VECTORS \
	(HQMV2_PF_NUM_NON_CQ_INTERRUPT_VECTORS + \
	 HQMV2_PF_NUM_CQ_INTERRUPT_VECTORS)
#define HQMV2_PF_NUM_COMPRESSED_MODE_VECTORS \
	(HQMV2_PF_NUM_NON_CQ_INTERRUPT_VECTORS + 1)
#define HQMV2_PF_NUM_PACKED_MODE_VECTORS \
	HQMV2_PF_TOTAL_NUM_INTERRUPT_VECTORS
#define HQMV2_PF_COMPRESSED_MODE_CQ_VECTOR_ID \
	HQMV2_PF_NUM_NON_CQ_INTERRUPT_VECTORS

/* HQM non-CQ interrupts (alarm, mailbox, WDT) */
#define HQMV2_INT_NON_CQ 0

#define HQMV2_ALARM_HW_SOURCE_SYS 0
#define HQMV2_ALARM_HW_SOURCE_HQM 1

#define HQMV2_ALARM_HW_UNIT_CHP 4

#define HQMV2_ALARM_SYS_AID_ILLEGAL_QID 3
#define HQMV2_ALARM_SYS_AID_DISABLED_QID 4
#define HQMV2_ALARM_SYS_AID_ILLEGAL_HCW 5
#define HQMV2_ALARM_HW_CHP_AID_ILLEGAL_ENQ 1
#define HQMV2_ALARM_HW_CHP_AID_EXCESS_TOKEN_POPS 2

#define HQMV2_VF_NUM_NON_CQ_INTERRUPT_VECTORS 1
#define HQMV2_VF_NUM_CQ_INTERRUPT_VECTORS 31
#define HQMV2_VF_BASE_CQ_VECTOR_ID 0
#define HQMV2_VF_LAST_CQ_VECTOR_ID 30
#define HQMV2_VF_MBOX_VECTOR_ID 31
#define HQMV2_VF_TOTAL_NUM_INTERRUPT_VECTORS \
	(HQMV2_VF_NUM_NON_CQ_INTERRUPT_VECTORS + \
	 HQMV2_VF_NUM_CQ_INTERRUPT_VECTORS)

#define HQMV2_VDEV_MAX_NUM_INTERRUPT_VECTORS \
	(HQMV2_MAX_NUM_LDB_PORTS + HQMV2_MAX_NUM_DIR_PORTS + 1)

/* Hardware-defined base addresses. Those prefixed 'HQMV2_DRV' are only used by
 * the PF driver.
 */
#define HQMV2_DRV_LDB_PP_BASE 0x2300000
#define HQMV2_DRV_LDB_PP_STRIDE 0x1000
#define HQMV2_DRV_LDB_PP_BOUND \
	(HQMV2_DRV_LDB_PP_BASE + HQMV2_DRV_LDB_PP_STRIDE * \
	 HQMV2_MAX_NUM_LDB_PORTS)
#define HQMV2_DRV_DIR_PP_BASE 0x2200000
#define HQMV2_DRV_DIR_PP_STRIDE 0x1000
#define HQMV2_DRV_DIR_PP_BOUND \
	(HQMV2_DRV_DIR_PP_BASE + HQMV2_DRV_DIR_PP_STRIDE * \
	 HQMV2_MAX_NUM_DIR_PORTS)
#define HQMV2_LDB_PP_BASE 0x2100000
#define HQMV2_LDB_PP_STRIDE 0x1000
#define HQMV2_LDB_PP_BOUND \
	(HQMV2_LDB_PP_BASE + HQMV2_LDB_PP_STRIDE * \
	 HQMV2_MAX_NUM_LDB_PORTS)
#define HQMV2_DIR_PP_BASE 0x2000000
#define HQMV2_DIR_PP_STRIDE 0x1000
#define HQMV2_DIR_PP_BOUND \
	(HQMV2_DIR_PP_BASE + HQMV2_DIR_PP_STRIDE * \
	 HQMV2_MAX_NUM_DIR_PORTS)

struct flow_ids {
	u32 base;
	u32 bound;
};

struct hqmv2_resource_id {
	u32 phys_id;
	u32 virt_id;
	u8 vdev_owned;
	u8 vdev_id;
};

struct hqmv2_freelist {
	u32 base;
	u32 bound;
	u32 offset;
};

static inline u32 hqmv2_freelist_count(struct hqmv2_freelist *list)
{
	return (list->bound - list->base) - list->offset;
}

struct hqmv2_hcw {
	u64 data;
	/* Word 3 */
	u16 opaque;
	u8 qid;
	u8 sched_type:2;
	u8 priority:3;
	u8 msg_type:3;
	/* Word 4 */
	u16 lock_id;
	u8 ts_flag:1;
	u8 rsvd1:2;
	u8 no_dec:1;
	u8 cmp_id:4;
	u8 cq_token:1;
	u8 qe_comp:1;
	u8 qe_frag:1;
	u8 qe_valid:1;
	u8 int_arm:1;
	u8 error:1;
	u8 rsvd:2;
};

struct hqmv2_ldb_queue {
	struct hqmv2_list_entry domain_list;
	struct hqmv2_list_entry func_list;
	struct hqmv2_resource_id id;
	struct hqmv2_resource_id domain_id;
	u32 num_qid_inflights;
	u32 aqed_limit;
	u8 sn_cfg_valid;
	u32 sn_group;
	u32 sn_slot;
	u32 num_mappings;
	u8 num_pending_additions;
	u8 owned;
	u8 configured;
};

/* Directed ports and queues are paired by nature, so the driver tracks them
 * with a single data structure.
 */
struct hqmv2_dir_pq_pair {
	struct hqmv2_list_entry domain_list;
	struct hqmv2_list_entry func_list;
	struct hqmv2_resource_id id;
	struct hqmv2_resource_id domain_id;
	u8 queue_configured;
	u8 port_configured;
	u8 owned;
	u8 enabled;
	u32 ref_cnt;
};

enum hqmv2_qid_map_state {
	/* The slot doesn't contain a valid queue mapping */
	HQMV2_QUEUE_UNMAPPED,
	/* The slot contains a valid queue mapping */
	HQMV2_QUEUE_MAPPED,
	/* The driver is mapping a queue into this slot */
	HQMV2_QUEUE_MAP_IN_PROG,
	/* The driver is unmapping a queue from this slot */
	HQMV2_QUEUE_UNMAP_IN_PROG,
	/* The driver is unmapping a queue from this slot, and once complete
	 * will replace it with another mapping.
	 */
	HQMV2_QUEUE_UNMAP_IN_PROG_PENDING_MAP,
};

struct hqmv2_ldb_port_qid_map {
	u16 qid;
	u8 priority;
	u16 pending_qid;
	u8 pending_priority;
	enum hqmv2_qid_map_state state;
};

struct hqmv2_ldb_port {
	struct hqmv2_list_entry domain_list;
	struct hqmv2_list_entry func_list;
	struct hqmv2_resource_id id;
	struct hqmv2_resource_id domain_id;
	u32 hist_list_entry_base;
	u32 hist_list_entry_limit;
	/* The qid_map represents the hardware QID mapping state. */
	struct hqmv2_ldb_port_qid_map qid_map[HQMV2_MAX_NUM_QIDS_PER_LDB_CQ];
	u32 ref_cnt;
	u8 num_pending_removals;
	u8 num_mappings;
	u8 owned;
	u8 enabled;
	u8 configured;
};

struct hqmv2_sn_group {
	u32 mode;
	u32 sequence_numbers_per_queue;
	u32 slot_use_bitmap;
	u32 id;
};

static inline bool hqmv2_sn_group_full(struct hqmv2_sn_group *group)
{
	u32 mask[] = {
		0x0000ffff,  /* 64 SNs per queue */
		0x000000ff,  /* 128 SNs per queue */
		0x0000000f,  /* 256 SNs per queue */
		0x00000003,  /* 512 SNs per queue */
		0x00000001}; /* 1024 SNs per queue */

	return group->slot_use_bitmap == mask[group->mode];
}

static inline int hqmv2_sn_group_alloc_slot(struct hqmv2_sn_group *group)
{
	u32 bound[6] = {16, 8, 4, 2, 1};
	int i;

	for (i = 0; i < bound[group->mode]; i++) {
		if (!(group->slot_use_bitmap & (1 << i))) {
			group->slot_use_bitmap |= 1 << i;
			return i;
		}
	}

	return -1;
}

static inline void
hqmv2_sn_group_free_slot(struct hqmv2_sn_group *group, int slot)
{
	group->slot_use_bitmap &= ~(1 << slot);
}

struct hqmv2_domain {
	struct hqmv2_function_resources *parent_func;
	struct hqmv2_list_entry func_list;
	struct hqmv2_list_head used_ldb_queues;
	struct hqmv2_list_head used_ldb_ports[HQMV2_NUM_COS_DOMAINS];
	struct hqmv2_list_head used_dir_pq_pairs;
	struct hqmv2_list_head avail_ldb_queues;
	struct hqmv2_list_head avail_ldb_ports[HQMV2_NUM_COS_DOMAINS];
	struct hqmv2_list_head avail_dir_pq_pairs;
	u32 total_hist_list_entries;
	u32 avail_hist_list_entries;
	u32 hist_list_entry_base;
	u32 hist_list_entry_offset;
	u32 num_ldb_credits;
	u32 num_dir_credits;
	u32 num_avail_aqed_entries;
	u32 num_used_aqed_entries;
	struct hqmv2_resource_id id;
	int num_pending_removals;
	int num_pending_additions;
	u8 configured;
	u8 started;
};

struct hqmv2_bitmap;

struct hqmv2_function_resources {
	u32 num_avail_domains;
	struct hqmv2_list_head avail_domains;
	struct hqmv2_list_head used_domains;
	u32 num_avail_ldb_queues;
	struct hqmv2_list_head avail_ldb_queues;
	u32 num_avail_ldb_ports[HQMV2_NUM_COS_DOMAINS];
	struct hqmv2_list_head avail_ldb_ports[HQMV2_NUM_COS_DOMAINS];
	u32 num_avail_dir_pq_pairs;
	struct hqmv2_list_head avail_dir_pq_pairs;
	u32 num_avail_qed_entries;
	u32 num_avail_dqed_entries;
	u32 num_avail_aqed_entries;
	struct hqmv2_bitmap *avail_hist_list_entries;
	u8 locked; /* (VDEV only) */
};

/* After initialization, each resource in hqmv2_hw_resources is located in one
 * of the following lists:
 * -- The PF's available resources list. These are unconfigured resources owned
 *	by the PF and not allocated to an HQMV2 scheduling domain.
 * -- A VDEV's available resources list. These are VDEV-owned unconfigured
 *	resources not allocated to an HQMV2 scheduling domain.
 * -- A domain's available resources list. These are domain-owned unconfigured
 *	resources.
 * -- A domain's used resources list. These are are domain-owned configured
 *	resources.
 *
 * A resource moves to a new list when a VDEV or domain is created or destroyed,
 * or when the resource is configured.
 */
struct hqmv2_hw_resources {
	struct hqmv2_ldb_queue ldb_queues[HQMV2_MAX_NUM_LDB_QUEUES];
	struct hqmv2_ldb_port ldb_ports[HQMV2_MAX_NUM_LDB_PORTS];
	struct hqmv2_dir_pq_pair dir_pq_pairs[HQMV2_MAX_NUM_DIR_PORTS];
	struct hqmv2_sn_group sn_groups[HQMV2_MAX_NUM_SEQUENCE_NUMBER_GROUPS];
};

struct hqmv2_mbox {
	u32 *mbox;
	u32 *isr_in_progress;
};

struct hqmv2_sw_mbox {
	struct hqmv2_mbox vdev_to_pf;
	struct hqmv2_mbox pf_to_vdev;
	void (*pf_to_vdev_inject)(void *arg);
	void *pf_to_vdev_inject_arg;
};

struct hqmv2_hw {
	/* BAR 0 address */
	void __iomem *csr_kva;
	unsigned long csr_phys_addr;
	/* BAR 2 address */
	void __iomem *func_kva;
	unsigned long func_phys_addr;

	/* Resource tracking */
	struct hqmv2_hw_resources rsrcs;
	struct hqmv2_function_resources pf;
	struct hqmv2_function_resources vdev[HQMV2_MAX_NUM_VDEVS];
	struct hqmv2_domain domains[HQMV2_MAX_NUM_DOMAINS];
	u8 cos_reservation[HQMV2_NUM_COS_DOMAINS];

	/* Virtualization */
	int virt_mode;
	struct hqmv2_sw_mbox mbox[HQMV2_MAX_NUM_VDEVS];
	unsigned int pasid[HQMV2_MAX_NUM_VDEVS];
};

#endif /* __HQMV2_HW_TYPES_H */
