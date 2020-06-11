/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
 * Copyright(c) 2016-2020 Intel Corporation
 */

#ifndef __DLB_BASE_DLB_MBOX_H
#define __DLB_BASE_DLB_MBOX_H

#include <linux/types.h>

#include "dlb_regs.h"

#define DLB_MBOX_INTERFACE_VERSION 1

/* The PF uses its PF->VF mailbox to send responses to VF requests, as well as
 * to send requests of its own (e.g. notifying a VF of an impending FLR).
 * To avoid communication race conditions, e.g. the PF sends a response and then
 * sends a request before the VF reads the response, the PF->VF mailbox is
 * divided into two sections:
 * - Bytes 0-47: PF responses
 * - Bytes 48-63: PF requests
 *
 * Partitioning the PF->VF mailbox allows responses and requests to occupy the
 * mailbox simultaneously.
 */
#define DLB_PF2VF_RESP_BYTES 48
#define DLB_PF2VF_RESP_BASE 0
#define DLB_PF2VF_RESP_BASE_WORD (DLB_PF2VF_RESP_BASE / 4)

#define DLB_PF2VF_REQ_BYTES \
	(DLB_FUNC_PF_PF2VF_MAILBOX_BYTES - DLB_PF2VF_RESP_BYTES)
#define DLB_PF2VF_REQ_BASE DLB_PF2VF_RESP_BYTES
#define DLB_PF2VF_REQ_BASE_WORD (DLB_PF2VF_REQ_BASE / 4)

/* Similarly, the VF->PF mailbox is divided into two sections:
 * - Bytes 0-239: VF requests
 * - Bytes 240-255: VF responses
 */
#define DLB_VF2PF_REQ_BYTES 240
#define DLB_VF2PF_REQ_BASE 0
#define DLB_VF2PF_REQ_BASE_WORD (DLB_VF2PF_REQ_BASE / 4)

#define DLB_VF2PF_RESP_BYTES \
	(DLB_FUNC_VF_VF2PF_MAILBOX_BYTES - DLB_VF2PF_REQ_BYTES)
#define DLB_VF2PF_RESP_BASE DLB_VF2PF_REQ_BYTES
#define DLB_VF2PF_RESP_BASE_WORD (DLB_VF2PF_RESP_BASE / 4)

/* VF-initiated commands */
enum dlb_mbox_cmd_type {
	DLB_MBOX_CMD_REGISTER,
	DLB_MBOX_CMD_UNREGISTER,
	DLB_MBOX_CMD_GET_NUM_RESOURCES,
	DLB_MBOX_CMD_CREATE_SCHED_DOMAIN,
	DLB_MBOX_CMD_RESET_SCHED_DOMAIN,
	DLB_MBOX_CMD_CREATE_LDB_POOL,
	DLB_MBOX_CMD_CREATE_DIR_POOL,
	DLB_MBOX_CMD_CREATE_LDB_QUEUE,
	DLB_MBOX_CMD_CREATE_DIR_QUEUE,
	DLB_MBOX_CMD_CREATE_LDB_PORT,
	DLB_MBOX_CMD_CREATE_DIR_PORT,
	DLB_MBOX_CMD_ENABLE_LDB_PORT,
	DLB_MBOX_CMD_DISABLE_LDB_PORT,
	DLB_MBOX_CMD_ENABLE_DIR_PORT,
	DLB_MBOX_CMD_DISABLE_DIR_PORT,
	DLB_MBOX_CMD_LDB_PORT_OWNED_BY_DOMAIN,
	DLB_MBOX_CMD_DIR_PORT_OWNED_BY_DOMAIN,
	DLB_MBOX_CMD_MAP_QID,
	DLB_MBOX_CMD_UNMAP_QID,
	DLB_MBOX_CMD_START_DOMAIN,
	DLB_MBOX_CMD_ENABLE_LDB_PORT_INTR,
	DLB_MBOX_CMD_ENABLE_DIR_PORT_INTR,
	DLB_MBOX_CMD_ARM_CQ_INTR,
	DLB_MBOX_CMD_GET_NUM_USED_RESOURCES,
	DLB_MBOX_CMD_INIT_CQ_SCHED_COUNT,
	DLB_MBOX_CMD_COLLECT_CQ_SCHED_COUNT,
	DLB_MBOX_CMD_ACK_VF_FLR_DONE,
	DLB_MBOX_CMD_GET_SN_ALLOCATION,
	DLB_MBOX_CMD_GET_LDB_QUEUE_DEPTH,
	DLB_MBOX_CMD_GET_DIR_QUEUE_DEPTH,
	DLB_MBOX_CMD_PENDING_PORT_UNMAPS,
	DLB_MBOX_CMD_QUERY_CQ_POLL_MODE,
	DLB_MBOX_CMD_GET_SN_OCCUPANCY,

	/* NUM_QE_CMD_TYPES must be last */
	NUM_DLB_MBOX_CMD_TYPES,
};

static const char dlb_mbox_cmd_type_strings[][128] = {
	"DLB_MBOX_CMD_REGISTER",
	"DLB_MBOX_CMD_UNREGISTER",
	"DLB_MBOX_CMD_GET_NUM_RESOURCES",
	"DLB_MBOX_CMD_CREATE_SCHED_DOMAIN",
	"DLB_MBOX_CMD_RESET_SCHED_DOMAIN",
	"DLB_MBOX_CMD_CREATE_LDB_POOL",
	"DLB_MBOX_CMD_CREATE_DIR_POOL",
	"DLB_MBOX_CMD_CREATE_LDB_QUEUE",
	"DLB_MBOX_CMD_CREATE_DIR_QUEUE",
	"DLB_MBOX_CMD_CREATE_LDB_PORT",
	"DLB_MBOX_CMD_CREATE_DIR_PORT",
	"DLB_MBOX_CMD_ENABLE_LDB_PORT",
	"DLB_MBOX_CMD_DISABLE_LDB_PORT",
	"DLB_MBOX_CMD_ENABLE_DIR_PORT",
	"DLB_MBOX_CMD_DISABLE_DIR_PORT",
	"DLB_MBOX_CMD_LDB_PORT_OWNED_BY_DOMAIN",
	"DLB_MBOX_CMD_DIR_PORT_OWNED_BY_DOMAIN",
	"DLB_MBOX_CMD_MAP_QID",
	"DLB_MBOX_CMD_UNMAP_QID",
	"DLB_MBOX_CMD_START_DOMAIN",
	"DLB_MBOX_CMD_ENABLE_LDB_PORT_INTR",
	"DLB_MBOX_CMD_ENABLE_DIR_PORT_INTR",
	"DLB_MBOX_CMD_ARM_CQ_INTR",
	"DLB_MBOX_CMD_GET_NUM_USED_RESOURCES",
	"DLB_MBOX_CMD_INIT_CQ_SCHED_COUNT",
	"DLB_MBOX_CMD_COLLECT_CQ_SCHED_COUNT",
	"DLB_MBOX_CMD_ACK_VF_FLR_DONE",
	"DLB_MBOX_CMD_GET_SN_ALLOCATION",
	"DLB_MBOX_CMD_GET_LDB_QUEUE_DEPTH",
	"DLB_MBOX_CMD_GET_DIR_QUEUE_DEPTH",
	"DLB_MBOX_CMD_PENDING_PORT_UNMAPS",
	"DLB_MBOX_CMD_QUERY_CQ_POLL_MODE",
	"DLB_MBOX_CMD_GET_SN_OCCUPANCY",
};

/* PF-initiated commands */
enum dlb_mbox_vf_cmd_type {
	DLB_MBOX_VF_CMD_DOMAIN_ALERT,
	DLB_MBOX_VF_CMD_NOTIFICATION,
	DLB_MBOX_VF_CMD_IN_USE,

	/* NUM_DLB_MBOX_VF_CMD_TYPES must be last */
	NUM_DLB_MBOX_VF_CMD_TYPES,
};

static const char dlb_mbox_vf_cmd_type_strings[][128] = {
	"DLB_MBOX_VF_CMD_DOMAIN_ALERT",
	"DLB_MBOX_VF_CMD_NOTIFICATION",
	"DLB_MBOX_VF_CMD_IN_USE",
};

#define DLB_MBOX_CMD_TYPE(hdr) \
	(((struct dlb_mbox_req_hdr *)hdr)->type)
#define DLB_MBOX_CMD_STRING(hdr) \
	dlb_mbox_cmd_type_strings[DLB_MBOX_CMD_TYPE(hdr)]

enum dlb_mbox_status_type {
	DLB_MBOX_ST_SUCCESS,
	DLB_MBOX_ST_INVALID_CMD_TYPE,
	DLB_MBOX_ST_VERSION_MISMATCH,
	DLB_MBOX_ST_EXPECTED_PHASE_ONE,
	DLB_MBOX_ST_EXPECTED_PHASE_TWO,
	DLB_MBOX_ST_INVALID_OWNER_VF,
};

static const char dlb_mbox_status_type_strings[][128] = {
	"DLB_MBOX_ST_SUCCESS",
	"DLB_MBOX_ST_INVALID_CMD_TYPE",
	"DLB_MBOX_ST_VERSION_MISMATCH",
	"DLB_MBOX_ST_EXPECTED_PHASE_ONE",
	"DLB_MBOX_ST_EXPECTED_PHASE_TWO",
	"DLB_MBOX_ST_INVALID_OWNER_VF",
};

#define DLB_MBOX_ST_TYPE(hdr) \
	(((struct dlb_mbox_resp_hdr *)hdr)->status)
#define DLB_MBOX_ST_STRING(hdr) \
	dlb_mbox_status_type_strings[DLB_MBOX_ST_TYPE(hdr)]

/* This structure is always the first field in a request structure */
struct dlb_mbox_req_hdr {
	u32 type;
};

/* This structure is always the first field in a response structure */
struct dlb_mbox_resp_hdr {
	u32 status;
};

struct dlb_mbox_register_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u16 min_interface_version;
	u16 max_interface_version;
};

struct dlb_mbox_register_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 interface_version;
	u8 pf_id;
	u8 vf_id;
	u8 is_auxiliary_vf;
	u8 primary_vf_id;
	u32 padding;
};

struct dlb_mbox_unregister_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 padding;
};

struct dlb_mbox_unregister_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 padding;
};

struct dlb_mbox_get_num_resources_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 padding;
};

struct dlb_mbox_get_num_resources_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u16 num_sched_domains;
	u16 num_ldb_queues;
	u16 num_ldb_ports;
	u16 num_dir_ports;
	u16 padding0;
	u8 num_ldb_credit_pools;
	u8 num_dir_credit_pools;
	u32 num_atomic_inflights;
	u32 max_contiguous_atomic_inflights;
	u32 num_hist_list_entries;
	u32 max_contiguous_hist_list_entries;
	u16 num_ldb_credits;
	u16 max_contiguous_ldb_credits;
	u16 num_dir_credits;
	u16 max_contiguous_dir_credits;
	u32 padding1;
};

struct dlb_mbox_create_sched_domain_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 num_ldb_queues;
	u32 num_ldb_ports;
	u32 num_dir_ports;
	u32 num_atomic_inflights;
	u32 num_hist_list_entries;
	u32 num_ldb_credits;
	u32 num_dir_credits;
	u32 num_ldb_credit_pools;
	u32 num_dir_credit_pools;
};

struct dlb_mbox_create_sched_domain_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 id;
};

struct dlb_mbox_reset_sched_domain_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 id;
};

struct dlb_mbox_reset_sched_domain_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
};

struct dlb_mbox_create_credit_pool_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
	u32 num_credits;
	u32 padding;
};

struct dlb_mbox_create_credit_pool_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 id;
};

struct dlb_mbox_create_ldb_queue_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
	u32 num_sequence_numbers;
	u32 num_qid_inflights;
	u32 num_atomic_inflights;
	u32 padding;
};

struct dlb_mbox_create_ldb_queue_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 id;
};

struct dlb_mbox_create_dir_queue_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 padding0;
};

struct dlb_mbox_create_dir_queue_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 id;
};

struct dlb_mbox_create_ldb_port_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
	u32 ldb_credit_pool_id;
	u32 dir_credit_pool_id;
	u64 pop_count_address;
	u16 ldb_credit_high_watermark;
	u16 ldb_credit_low_watermark;
	u16 ldb_credit_quantum;
	u16 dir_credit_high_watermark;
	u16 dir_credit_low_watermark;
	u16 dir_credit_quantum;
	u32 padding0;
	u16 cq_depth;
	u16 cq_history_list_size;
	u32 padding1;
	u64 cq_base_address;
	u64 nq_base_address;
	u32 nq_size;
	u32 padding2;
};

struct dlb_mbox_create_ldb_port_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 id;
};

struct dlb_mbox_create_dir_port_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
	u32 ldb_credit_pool_id;
	u32 dir_credit_pool_id;
	u64 pop_count_address;
	u16 ldb_credit_high_watermark;
	u16 ldb_credit_low_watermark;
	u16 ldb_credit_quantum;
	u16 dir_credit_high_watermark;
	u16 dir_credit_low_watermark;
	u16 dir_credit_quantum;
	u16 cq_depth;
	u16 padding0;
	u64 cq_base_address;
	s32 queue_id;
	u32 padding1;
};

struct dlb_mbox_create_dir_port_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 id;
};

struct dlb_mbox_enable_ldb_port_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 padding;
};

struct dlb_mbox_enable_ldb_port_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding;
};

struct dlb_mbox_disable_ldb_port_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 padding;
};

struct dlb_mbox_disable_ldb_port_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding;
};

struct dlb_mbox_enable_dir_port_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 padding;
};

struct dlb_mbox_enable_dir_port_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding;
};

struct dlb_mbox_disable_dir_port_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 padding;
};

struct dlb_mbox_disable_dir_port_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding;
};

struct dlb_mbox_ldb_port_owned_by_domain_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 padding;
};

struct dlb_mbox_ldb_port_owned_by_domain_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	s32 owned;
};

struct dlb_mbox_dir_port_owned_by_domain_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 padding;
};

struct dlb_mbox_dir_port_owned_by_domain_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	s32 owned;
};

struct dlb_mbox_map_qid_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 qid;
	u32 priority;
	u32 padding0;
};

struct dlb_mbox_map_qid_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 id;
};

struct dlb_mbox_unmap_qid_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 qid;
};

struct dlb_mbox_unmap_qid_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding;
};

struct dlb_mbox_start_domain_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
};

struct dlb_mbox_start_domain_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding;
};

struct dlb_mbox_enable_ldb_port_intr_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u16 port_id;
	u16 thresh;
	u16 vector;
	u16 owner_vf;
	u16 reserved[2];
};

struct dlb_mbox_enable_ldb_port_intr_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding0;
};

struct dlb_mbox_enable_dir_port_intr_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u16 port_id;
	u16 thresh;
	u16 vector;
	u16 owner_vf;
	u16 reserved[2];
};

struct dlb_mbox_enable_dir_port_intr_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding0;
};

struct dlb_mbox_arm_cq_intr_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 is_ldb;
};

struct dlb_mbox_arm_cq_intr_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding0;
};

/* The alert_id and aux_alert_data follows the format of the alerts defined in
 * dlb_types.h. The alert id contains an enum dlb_domain_alert_id value, and
 * the aux_alert_data value varies depending on the alert.
 */
struct dlb_mbox_vf_alert_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
	u32 alert_id;
	u32 aux_alert_data;
};

enum dlb_mbox_vf_notification_type {
	DLB_MBOX_VF_NOTIFICATION_PRE_RESET,
	DLB_MBOX_VF_NOTIFICATION_POST_RESET,

	/* NUM_DLB_MBOX_VF_NOTIFICATION_TYPES must be last */
	NUM_DLB_MBOX_VF_NOTIFICATION_TYPES,
};

struct dlb_mbox_vf_notification_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 notification;
};

struct dlb_mbox_vf_in_use_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 padding;
};

struct dlb_mbox_vf_in_use_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 in_use;
};

struct dlb_mbox_ack_vf_flr_done_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 padding;
};

struct dlb_mbox_ack_vf_flr_done_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding;
};

struct dlb_mbox_get_sn_allocation_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 group_id;
};

struct dlb_mbox_get_sn_allocation_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 num;
};

struct dlb_mbox_get_ldb_queue_depth_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
	u32 queue_id;
	u32 padding;
};

struct dlb_mbox_get_ldb_queue_depth_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 depth;
};

struct dlb_mbox_get_dir_queue_depth_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
	u32 queue_id;
	u32 padding;
};

struct dlb_mbox_get_dir_queue_depth_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 depth;
};

struct dlb_mbox_pending_port_unmaps_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 padding;
};

struct dlb_mbox_pending_port_unmaps_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 num;
};

struct dlb_mbox_query_cq_poll_mode_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 padding;
};

struct dlb_mbox_query_cq_poll_mode_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 mode;
};

struct dlb_mbox_get_sn_occupancy_cmd_req {
	struct dlb_mbox_req_hdr hdr;
	u32 group_id;
};

struct dlb_mbox_get_sn_occupancy_cmd_resp {
	struct dlb_mbox_resp_hdr hdr;
	u32 num;
};

#endif /* __DLB_BASE_DLB_MBOX_H */
