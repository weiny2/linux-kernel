/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
 * Copyright(c) 2016-2020 Intel Corporation
 */

#ifndef __DLB_HW_TYPES_H
#define __DLB_HW_TYPES_H

#include <linux/io.h>
#include <linux/types.h>

#define DLB_PCI_REG_READ(addr)        ioread32(addr)
#define DLB_PCI_REG_WRITE(reg, value) iowrite32(value, reg)

/* Read/write register 'reg' in the CSR BAR space */
#define DLB_CSR_REG_ADDR(a, reg) ((a)->csr_kva + (reg))
#define DLB_CSR_RD(hw, reg) \
	DLB_PCI_REG_READ(DLB_CSR_REG_ADDR((hw), (reg)))
#define DLB_CSR_WR(hw, reg, value) \
	DLB_PCI_REG_WRITE(DLB_CSR_REG_ADDR((hw), (reg)), (value))

/* Read/write register 'reg' in the func BAR space */
#define DLB_FUNC_REG_ADDR(a, reg) ((a)->func_kva + (reg))
#define DLB_FUNC_RD(hw, reg) \
	DLB_PCI_REG_READ(DLB_FUNC_REG_ADDR((hw), (reg)))
#define DLB_FUNC_WR(hw, reg, value) \
	DLB_PCI_REG_WRITE(DLB_FUNC_REG_ADDR((hw), (reg)), (value))

#define DLB_MAX_NUM_VFS 16
#define DLB_MAX_NUM_DOMAINS 32
#define DLB_MAX_NUM_LDB_QUEUES 128
#define DLB_MAX_NUM_LDB_PORTS 64
#define DLB_MAX_NUM_DIR_PORTS 128
#define DLB_MAX_NUM_LDB_CREDITS 16384
#define DLB_MAX_NUM_DIR_CREDITS 4096
#define DLB_MAX_NUM_LDB_CREDIT_POOLS 64
#define DLB_MAX_NUM_DIR_CREDIT_POOLS 64
#define DLB_MAX_NUM_HIST_LIST_ENTRIES 5120
#define DLB_MAX_NUM_AQOS_ENTRIES 2048
#define DLB_MAX_NUM_TOTAL_OUTSTANDING_COMPLETIONS 4096
#define DLB_MAX_NUM_QIDS_PER_LDB_CQ 8
#define DLB_MAX_NUM_SEQUENCE_NUMBER_GROUPS 4
#define DLB_MAX_NUM_SEQUENCE_NUMBER_MODES 6
#define DLB_QID_PRIORITIES 8
#define DLB_NUM_ARB_WEIGHTS 8
#define DLB_MAX_WEIGHT 255
#define DLB_MAX_PORT_CREDIT_QUANTUM 1023
#define DLB_MAX_CQ_COMP_CHECK_LOOPS 409600
#define DLB_MAX_QID_EMPTY_CHECK_LOOPS (32 * 64 * 1024 * (800 / 30))
#define DLB_HZ 800000000

/* Used for DLB A-stepping workaround for hardware write buffer lock up issue */
#define DLB_A_STEP_MAX_PORTS 128

#define DLB_PF_DEV_ID 0x270B
#define DLB_VF_DEV_ID 0x270C

/* Interrupt related macros */
#define DLB_PF_NUM_NON_CQ_INTERRUPT_VECTORS 8
#define DLB_PF_NUM_CQ_INTERRUPT_VECTORS	 64
#define DLB_PF_TOTAL_NUM_INTERRUPT_VECTORS \
	(DLB_PF_NUM_NON_CQ_INTERRUPT_VECTORS + \
	 DLB_PF_NUM_CQ_INTERRUPT_VECTORS)
#define DLB_PF_NUM_COMPRESSED_MODE_VECTORS \
	(DLB_PF_NUM_NON_CQ_INTERRUPT_VECTORS + 1)
#define DLB_PF_NUM_PACKED_MODE_VECTORS	 DLB_PF_TOTAL_NUM_INTERRUPT_VECTORS
#define DLB_PF_COMPRESSED_MODE_CQ_VECTOR_ID DLB_PF_NUM_NON_CQ_INTERRUPT_VECTORS

#define DLB_VF_NUM_NON_CQ_INTERRUPT_VECTORS 1
#define DLB_VF_NUM_CQ_INTERRUPT_VECTORS 31
#define DLB_VF_BASE_CQ_VECTOR_ID 0
#define DLB_VF_LAST_CQ_VECTOR_ID 30
#define DLB_VF_MBOX_VECTOR_ID 31
#define DLB_VF_TOTAL_NUM_INTERRUPT_VECTORS \
	(DLB_VF_NUM_NON_CQ_INTERRUPT_VECTORS + \
	 DLB_VF_NUM_CQ_INTERRUPT_VECTORS)

#define DLB_PF_NUM_ALARM_INTERRUPT_VECTORS 4
/* DLB ALARM interrupts */
#define DLB_INT_ALARM 0
/* VF to PF Mailbox Service Request */
#define DLB_INT_VF_TO_PF_MBOX 1
/* HCW Ingress Errors */
#define DLB_INT_INGRESS_ERROR 3

#define DLB_ALARM_HW_SOURCE_SYS 0
#define DLB_ALARM_HW_SOURCE_DLB 1

#define DLB_ALARM_HW_UNIT_CHP 1
#define DLB_ALARM_HW_UNIT_LSP 3

#define DLB_ALARM_HW_CHP_AID_OUT_OF_CREDITS 6
#define DLB_ALARM_HW_CHP_AID_ILLEGAL_ENQ 7
#define DLB_ALARM_HW_LSP_AID_EXCESS_TOKEN_POPS 15
#define DLB_ALARM_SYS_AID_ILLEGAL_HCW 0
#define DLB_ALARM_SYS_AID_ILLEGAL_QID 3
#define DLB_ALARM_SYS_AID_DISABLED_QID 4
#define DLB_ALARM_SYS_AID_ILLEGAL_CQID 6

/* Hardware-defined base addresses */
#define DLB_LDB_PP_BASE 0x2100000
#define DLB_LDB_PP_STRIDE 0x1000
#define DLB_LDB_PP_BOUND \
	(DLB_LDB_PP_BASE + DLB_LDB_PP_STRIDE * DLB_MAX_NUM_LDB_PORTS)
#define DLB_DIR_PP_BASE 0x2000000
#define DLB_DIR_PP_STRIDE 0x1000
#define DLB_DIR_PP_BOUND \
	(DLB_DIR_PP_BASE + DLB_DIR_PP_STRIDE * DLB_MAX_NUM_DIR_PORTS)

struct dlb_resource_id {
	u32 phys_id;
	u32 virt_id;
	u8 vf_owned;
	u8 vf_id;
};

struct dlb_freelist {
	u32 base;
	u32 bound;
	u32 offset;
};

static inline u32 dlb_freelist_count(struct dlb_freelist *list)
{
	return (list->bound - list->base) - list->offset;
}

struct dlb_hcw {
	u64 data;
	/* Word 3 */
	u16 opaque;
	u8 qid;
	u8 sched_type:2;
	u8 priority:3;
	u8 msg_type:3;
	/* Word 4 */
	u16 lock_id;
	u8 meas_lat:1;
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

struct dlb_ldb_queue {
	struct list_head domain_list;
	struct list_head func_list;
	struct dlb_resource_id id;
	struct dlb_resource_id domain_id;
	u32 num_qid_inflights;
	struct dlb_freelist aqed_freelist;
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
struct dlb_dir_pq_pair {
	struct list_head domain_list;
	struct list_head func_list;
	struct dlb_resource_id id;
	struct dlb_resource_id domain_id;
	u8 ldb_pool_used;
	u8 dir_pool_used;
	u8 queue_configured;
	u8 port_configured;
	u8 owned;
	u8 enabled;
	u32 ref_cnt;
};

enum dlb_qid_map_state {
	/* The slot doesn't contain a valid queue mapping */
	DLB_QUEUE_UNMAPPED,
	/* The slot contains a valid queue mapping */
	DLB_QUEUE_MAPPED,
	/* The driver is mapping a queue into this slot */
	DLB_QUEUE_MAP_IN_PROGRESS,
	/* The driver is unmapping a queue from this slot */
	DLB_QUEUE_UNMAP_IN_PROGRESS,
	/* The driver is unmapping a queue from this slot, and once complete
	 * will replace it with another mapping.
	 */
	DLB_QUEUE_UNMAP_IN_PROGRESS_PENDING_MAP,
};

struct dlb_ldb_port_qid_map {
	u16 qid;
	u8 priority;
	u16 pending_qid;
	u8 pending_priority;
	enum dlb_qid_map_state state;
};

struct dlb_ldb_port {
	struct list_head domain_list;
	struct list_head func_list;
	struct dlb_resource_id id;
	struct dlb_resource_id domain_id;
	u8 ldb_pool_used;
	u8 dir_pool_used;
	u8 init_tkn_cnt;
	u32 hist_list_entry_base;
	u32 hist_list_entry_limit;
	/* The qid_map represents the hardware QID mapping state. */
	struct dlb_ldb_port_qid_map qid_map[DLB_MAX_NUM_QIDS_PER_LDB_CQ];
	u32 ref_cnt;
	u8 num_pending_removals;
	u8 num_mappings;
	u8 owned;
	u8 enabled;
	u8 configured;
};

struct dlb_credit_pool {
	struct list_head domain_list;
	struct list_head func_list;
	struct dlb_resource_id id;
	struct dlb_resource_id domain_id;
	u32 total_credits;
	u32 avail_credits;
	u8 owned;
	u8 configured;
};

struct dlb_sn_group {
	u32 mode;
	u32 sequence_numbers_per_queue;
	u32 slot_use_bitmap;
	u32 id;
};

static inline bool dlb_sn_group_full(struct dlb_sn_group *group)
{
	u32 mask[6] = {
		0xffffffff,  /* 32 SNs per queue */
		0x0000ffff,  /* 64 SNs per queue */
		0x000000ff,  /* 128 SNs per queue */
		0x0000000f,  /* 256 SNs per queue */
		0x00000003,  /* 512 SNs per queue */
		0x00000001}; /* 1024 SNs per queue */

	return group->slot_use_bitmap == mask[group->mode];
}

static inline int dlb_sn_group_alloc_slot(struct dlb_sn_group *group)
{
	int bound[6] = {32, 16, 8, 4, 2, 1};
	int i;

	for (i = 0; i < bound[group->mode]; i++) {
		if (!(group->slot_use_bitmap & (1 << i))) {
			group->slot_use_bitmap |= 1 << i;
			return i;
		}
	}

	return -1;
}

static inline void dlb_sn_group_free_slot(struct dlb_sn_group *group, int slot)
{
	group->slot_use_bitmap &= ~(1 << slot);
}

static inline int dlb_sn_group_used_slots(struct dlb_sn_group *group)
{
	int i, cnt = 0;

	for (i = 0; i < 32; i++)
		cnt += !!(group->slot_use_bitmap & (1 << i));

	return cnt;
}

struct dlb_domain {
	struct dlb_function_resources *parent_func;
	struct list_head func_list;
	struct list_head used_ldb_queues;
	struct list_head used_ldb_ports;
	struct list_head used_dir_pq_pairs;
	struct list_head used_ldb_credit_pools;
	struct list_head used_dir_credit_pools;
	struct list_head avail_ldb_queues;
	struct list_head avail_ldb_ports;
	struct list_head avail_dir_pq_pairs;
	struct list_head avail_ldb_credit_pools;
	struct list_head avail_dir_credit_pools;
	u32 total_hist_list_entries;
	u32 avail_hist_list_entries;
	u32 hist_list_entry_base;
	u32 hist_list_entry_offset;
	struct dlb_freelist qed_freelist;
	struct dlb_freelist dqed_freelist;
	struct dlb_freelist aqed_freelist;
	struct dlb_resource_id id;
	int num_pending_removals;
	int num_pending_additions;
	u8 configured;
	u8 started;
};

struct dlb_bitmap;

struct dlb_function_resources {
	u32 num_avail_domains;
	struct list_head avail_domains;
	struct list_head used_domains;
	u32 num_avail_ldb_queues;
	struct list_head avail_ldb_queues;
	u32 num_avail_ldb_ports;
	struct list_head avail_ldb_ports;
	u32 num_avail_dir_pq_pairs;
	struct list_head avail_dir_pq_pairs;
	struct dlb_bitmap *avail_hist_list_entries;
	struct dlb_bitmap *avail_qed_freelist_entries;
	struct dlb_bitmap *avail_dqed_freelist_entries;
	struct dlb_bitmap *avail_aqed_freelist_entries;
	u32 num_avail_ldb_credit_pools;
	struct list_head avail_ldb_credit_pools;
	u32 num_avail_dir_credit_pools;
	struct list_head avail_dir_credit_pools;
	u32 num_enabled_ldb_ports; /* (PF only) */
	u8 locked; /* (VF only) */
};

/* After initialization, each resource in dlb_hw_resources is located in one of
 * the following lists:
 * -- The PF's available resources list. These are unconfigured resources owned
 *	by the PF and not allocated to an DLB scheduling domain.
 * -- A VF's available resources list. These are VF-owned unconfigured
 *	resources not allocated to an DLB scheduling domain.
 * -- A domain's available resources list. These are domain-owned unconfigured
 *	resources.
 * -- A domain's used resources list. These are are domain-owned configured
 *	resources.
 *
 * A resource moves to a new list when a VF or domain is created or destroyed,
 * or when the resource is configured.
 */
struct dlb_hw_resources {
	struct dlb_ldb_queue ldb_queues[DLB_MAX_NUM_LDB_QUEUES];
	struct dlb_ldb_port ldb_ports[DLB_MAX_NUM_LDB_PORTS];
	struct dlb_dir_pq_pair dir_pq_pairs[DLB_MAX_NUM_DIR_PORTS];
	struct dlb_credit_pool ldb_credit_pools[DLB_MAX_NUM_LDB_CREDIT_POOLS];
	struct dlb_credit_pool dir_credit_pools[DLB_MAX_NUM_DIR_CREDIT_POOLS];
	struct dlb_sn_group sn_groups[DLB_MAX_NUM_SEQUENCE_NUMBER_GROUPS];
};

struct dlb_hw {
	/* BAR 0 address */
	void __iomem *csr_kva;
	unsigned long csr_phys_addr;
	/* BAR 2 address */
	void __iomem *func_kva;
	unsigned long func_phys_addr;
	/* Resource tracking */
	struct dlb_hw_resources rsrcs;
	struct dlb_function_resources pf;
	struct dlb_function_resources vf[DLB_MAX_NUM_VFS];
	struct dlb_domain domains[DLB_MAX_NUM_DOMAINS];
};

#endif /* __DLB_HW_TYPES_H */
