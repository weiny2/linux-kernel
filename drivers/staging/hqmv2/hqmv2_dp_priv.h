/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2017-2018 Intel Corporation
 */

#ifndef __HQMV2_PRIV_DP_H
#define __HQMV2_PRIV_DP_H

#include <linux/list.h>
#include "base/hqmv2_hw_types.h"

/* HQM related macros */
#define NUM_PORT_TYPES 2
#define BYTES_PER_CQ_ENTRY 16
#define PP_BASE(type) ((type == LDB) ? HQMV2_LDB_PP_BASE : HQMV2_DIR_PP_BASE)
#define NUM_QID_INFLIGHTS 4096
/* There are 32 LDB queues and 2K atomic inflights, and we evenly divide them
 * among the queues.
 */
#define NUM_ATM_INFLIGHTS_PER_LDB_QUEUE 64
#define NUM_LDB_CREDIT_POOLS 64
#define NUM_DIR_CREDIT_POOLS 64

#define HQMV2_SW_CREDIT_BATCH_SZ 32

/* Memory system related macros */
#define CACHE_LINE_SIZE 64
#define CACHE_LINE_MASK (CACHE_LINE_SIZE - 1)

/* DEBUG_ONLY() is used for statements that the compiler couldn't otherwise
 * optimize.
 */
#ifndef DISABLE_CHECK
#define DEBUG_ONLY(x) x
#else
#define DEBUG_ONLY(x)
#endif

#define HQMV2_MAGIC_NUM  0xBEEFFACE
#define DOMAIN_MAGIC_NUM  0x12344321
#define PORT_MAGIC_NUM 0x43211234

/*************************/
/** HQM port structures **/
/*************************/

struct hqmv2_port_hdl {
	struct list_head list;
	u32 magic_num;
	struct hqmv2_port *port;
	/* Cache line's worth of QEs (4) */
	struct hqmv2_enqueue_qe *qe;
};

enum hqmv2_port_type {
	LDB,
	DIR,
};

struct hqmv2_port {
	/* PP-related fields */
	u64 *pp_addr;
	atomic_t *credit_pool[NUM_PORT_TYPES];
	u16 num_credits[NUM_PORT_TYPES];

	void (*enqueue_four)(void *qe4, void *pp_addr);

	/* CQ-related fields */
	int cq_idx;
	int cq_depth;
	u8 cq_gen;
	struct hqmv2_dequeue_qe *cq_base;
	u16 owed_tokens;
	u16 owed_releases;
	u8 int_armed;

	/* Misc */
	int id;
	struct hqmv2_dp_domain *domain;
	enum hqmv2_port_type type;
	struct list_head hdl_list_head;
	/* resource_mutex protects port data during configuration operations */
	struct mutex resource_mutex;
	u8 enabled;
	u8 configured;
};

/***************************/
/** HQM Domain structures **/
/***************************/

struct hqmv2_domain_hdl {
	struct list_head list;
	u32 magic_num;
	struct hqmv2_dp_domain *domain;
};

enum hqmv2_domain_user_alert {
	HQMV2_DOMAIN_USER_ALERT_RESET,
};

struct hqmv2_domain_alert_thread {
	void (*fn)(void *alert, int domain_id, void *arg);
	void *arg;
	u8 started;
};

struct hqmv2_sw_credit_pool {
	u8 configured;
	atomic_t avail_credits;
};

struct hqmv2_sw_credits {
	u32 avail_credits[NUM_PORT_TYPES];
	struct hqmv2_sw_credit_pool ldb_pools[NUM_LDB_CREDIT_POOLS];
	struct hqmv2_sw_credit_pool dir_pools[NUM_DIR_CREDIT_POOLS];
};

struct hqmv2_dp_domain {
	int id;
	struct hqmv2_dev *dev;
	u8 shutdown;
	struct hqmv2_port ldb_ports[HQMV2_MAX_NUM_LDB_PORTS];
	struct hqmv2_port dir_ports[HQMV2_MAX_NUM_DIR_PORTS];
	u8 queue_valid[NUM_PORT_TYPES][HQMV2_MAX_NUM_LDB_QUEUES];
	struct hqmv2_sw_credits sw_credits;
	u8 reads_allowed;
	unsigned int num_readers;
	struct hqmv2_domain_alert_thread thread;
	struct hqmv2_dp *hqmv2_dp;
	/* resource_mutex protects domain data during configuration ops */
	struct mutex resource_mutex;
	u8 configured;
	u8 started;
	struct list_head hdl_list_head;
};

#define DEV_FROM_HQMV2_DP_DOMAIN(dom) (&(dom)->dev->pdev->dev)

/********************/
/** HQM structures **/
/********************/

struct hqmv2_dp {
	struct list_head next;
	u32 magic_num;
	int id;
#ifdef CONFIG_PM
	int pm_refcount;
#endif
	struct hqmv2_dev *dev;
	/* resource_mutex protects device data during configuration ops */
	struct mutex resource_mutex;
	struct hqmv2_dp_domain domains[HQMV2_MAX_NUM_DOMAINS];
};

/***************************/
/** "Advanced" structures **/
/***************************/

/* Possible future work: Expose advanced port creation functions to allow
 * expert users to provide their own memory space for CQ and PC and their
 * own credit configurations.
 */
struct hqmv2_create_port_adv {
	/* CQ base address */
	uintptr_t cq_base;
	/* History list size */
	u16 cq_history_list_size;
	/* Popcount base address */
	uintptr_t pc_base;
	/* Load-balanced credit low watermark */
	u16 ldb_credit_low_watermark;
	/* Load-balanced credit quantum */
	u16 ldb_credit_quantum;
	/* Directed credit low watermark */
	u16 dir_credit_low_watermark;
	/* Directed credit quantum */
	u16 dir_credit_quantum;
};

/*******************/
/** QE structures **/
/*******************/

#define CMD_ARM 5

struct hqmv2_enqueue_cmd_info {
	u8 qe_cmd:4;
	u8 int_arm:1;
	u8 error:1;
	u8 rsvd:2;
} __packed;

struct hqmv2_enqueue_qe {
	u64 data;
	u16 opaque;
	u8 qid;
	u8 sched_byte;
	u16 flow_id;
	u8 meas_lat:1;
	u8 rsvd1:2;
	u8 no_dec:1;
	u8 cmp_id:4;
	union {
		struct hqmv2_enqueue_cmd_info cmd_info;
		u8 cmd_byte;
	};
} __packed;

struct hqmv2_dequeue_qe {
	u64 data;
	u16 opaque;
	u8 qid;
	u8 sched_byte;
	u16 pp_id:10;
	u16 rsvd0:6;
	u8 debug;
	u8 cq_gen:1;
	u8 qid_depth:1;
	u8 rsvd1:3;
	u8 error:1;
	u8 rsvd2:2;
} __packed;

void hqmv2_datapath_init(struct hqmv2_dev *dev, int id);
void hqmv2_datapath_free(int id);

#endif /* __HQMV2_PRIV_DP_H */
