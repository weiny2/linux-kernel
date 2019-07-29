/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
 * Copyright(c) 2016-2018 Intel Corporation
 */

#ifndef __HQMV2_BASE_HQMV2_MBOX_H
#define __HQMV2_BASE_HQMV2_MBOX_H

#include "hqmv2_regs.h"
#include "hqmv2_osdep_types.h"

#define HQMV2_MBOX_INTERFACE_VERSION 1

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
#define HQMV2_PF2VF_RESP_BYTES 48
#define HQMV2_PF2VF_RESP_BASE 0
#define HQMV2_PF2VF_RESP_BASE_WORD (HQMV2_PF2VF_RESP_BASE / 4)

#define HQMV2_PF2VF_REQ_BYTES \
	(HQMV2_FUNC_PF_PF2VF_MAILBOX_BYTES - HQMV2_PF2VF_RESP_BYTES)
#define HQMV2_PF2VF_REQ_BASE HQMV2_PF2VF_RESP_BYTES
#define HQMV2_PF2VF_REQ_BASE_WORD (HQMV2_PF2VF_REQ_BASE / 4)

/* Similarly, the VF->PF mailbox is divided into two sections:
 * - Bytes 0-239: VF requests
 * - Bytes 240-255: VF responses
 */
#define HQMV2_VF2PF_REQ_BYTES 240
#define HQMV2_VF2PF_REQ_BASE 0
#define HQMV2_VF2PF_REQ_BASE_WORD (HQMV2_VF2PF_REQ_BASE / 4)

#define HQMV2_VF2PF_RESP_BYTES \
	(HQMV2_FUNC_VF_VF2PF_MAILBOX_BYTES - HQMV2_VF2PF_REQ_BYTES)
#define HQMV2_VF2PF_RESP_BASE HQMV2_VF2PF_REQ_BYTES
#define HQMV2_VF2PF_RESP_BASE_WORD (HQMV2_VF2PF_RESP_BASE / 4)

/* VF-initiated commands */
enum hqmv2_mbox_cmd_type {
	HQMV2_MBOX_CMD_REGISTER,
	HQMV2_MBOX_CMD_UNREGISTER,
	HQMV2_MBOX_CMD_GET_NUM_RESOURCES,
	HQMV2_MBOX_CMD_CREATE_SCHED_DOMAIN,
	HQMV2_MBOX_CMD_RESET_SCHED_DOMAIN,
	HQMV2_MBOX_CMD_CREATE_LDB_QUEUE,
	HQMV2_MBOX_CMD_CREATE_DIR_QUEUE,
	HQMV2_MBOX_CMD_CREATE_LDB_PORT,
	HQMV2_MBOX_CMD_CREATE_DIR_PORT,
	HQMV2_MBOX_CMD_ENABLE_LDB_PORT,
	HQMV2_MBOX_CMD_DISABLE_LDB_PORT,
	HQMV2_MBOX_CMD_ENABLE_DIR_PORT,
	HQMV2_MBOX_CMD_DISABLE_DIR_PORT,
	HQMV2_MBOX_CMD_LDB_PORT_OWNED_BY_DOMAIN,
	HQMV2_MBOX_CMD_DIR_PORT_OWNED_BY_DOMAIN,
	HQMV2_MBOX_CMD_MAP_QID,
	HQMV2_MBOX_CMD_UNMAP_QID,
	HQMV2_MBOX_CMD_START_DOMAIN,
	HQMV2_MBOX_CMD_ENABLE_LDB_PORT_INTR,
	HQMV2_MBOX_CMD_ENABLE_DIR_PORT_INTR,
	HQMV2_MBOX_CMD_ARM_CQ_INTR,
	HQMV2_MBOX_CMD_GET_NUM_USED_RESOURCES,
	HQMV2_MBOX_CMD_INIT_CQ_SCHED_COUNT,
	HQMV2_MBOX_CMD_COLLECT_CQ_SCHED_COUNT,
	HQMV2_MBOX_CMD_GET_SN_ALLOCATION,
	HQMV2_MBOX_CMD_GET_LDB_QUEUE_DEPTH,
	HQMV2_MBOX_CMD_GET_DIR_QUEUE_DEPTH,
	HQMV2_MBOX_CMD_PENDING_PORT_UNMAPS,
	HQMV2_MBOX_CMD_GET_COS_BW,

	/* NUM_QE_CMD_TYPES must be last */
	NUM_HQMV2_MBOX_CMD_TYPES,
};

static const char hqmv2_mbox_cmd_type_strings[][128] = {
	"HQMV2_MBOX_CMD_REGISTER",
	"HQMV2_MBOX_CMD_UNREGISTER",
	"HQMV2_MBOX_CMD_GET_NUM_RESOURCES",
	"HQMV2_MBOX_CMD_CREATE_SCHED_DOMAIN",
	"HQMV2_MBOX_CMD_RESET_SCHED_DOMAIN",
	"HQMV2_MBOX_CMD_CREATE_LDB_QUEUE",
	"HQMV2_MBOX_CMD_CREATE_DIR_QUEUE",
	"HQMV2_MBOX_CMD_CREATE_LDB_PORT",
	"HQMV2_MBOX_CMD_CREATE_DIR_PORT",
	"HQMV2_MBOX_CMD_ENABLE_LDB_PORT",
	"HQMV2_MBOX_CMD_DISABLE_LDB_PORT",
	"HQMV2_MBOX_CMD_ENABLE_DIR_PORT",
	"HQMV2_MBOX_CMD_DISABLE_DIR_PORT",
	"HQMV2_MBOX_CMD_LDB_PORT_OWNED_BY_DOMAIN",
	"HQMV2_MBOX_CMD_DIR_PORT_OWNED_BY_DOMAIN",
	"HQMV2_MBOX_CMD_MAP_QID",
	"HQMV2_MBOX_CMD_UNMAP_QID",
	"HQMV2_MBOX_CMD_START_DOMAIN",
	"HQMV2_MBOX_CMD_ENABLE_LDB_PORT_INTR",
	"HQMV2_MBOX_CMD_ENABLE_DIR_PORT_INTR",
	"HQMV2_MBOX_CMD_ARM_CQ_INTR",
	"HQMV2_MBOX_CMD_GET_NUM_USED_RESOURCES",
	"HQMV2_MBOX_CMD_INIT_CQ_SCHED_COUNT",
	"HQMV2_MBOX_CMD_COLLECT_CQ_SCHED_COUNT",
	"HQMV2_MBOX_CMD_GET_SN_ALLOCATION",
	"HQMV2_MBOX_CMD_GET_LDB_QUEUE_DEPTH",
	"HQMV2_MBOX_CMD_GET_DIR_QUEUE_DEPTH",
	"HQMV2_MBOX_CMD_PENDING_PORT_UNMAPS",
	"HQMV2_MBOX_CMD_GET_COS_BW",
};

/* PF-initiated commands */
enum hqmv2_mbox_vf_cmd_type {
	HQMV2_MBOX_VF_CMD_DOMAIN_ALERT,
	HQMV2_MBOX_VF_CMD_NOTIFICATION,
	HQMV2_MBOX_VF_CMD_IN_USE,

	/* NUM_HQMV2_MBOX_VF_CMD_TYPES must be last */
	NUM_HQMV2_MBOX_VF_CMD_TYPES,
};

static const char hqmv2_mbox_vf_cmd_type_strings[][128] = {
	"HQMV2_MBOX_VF_CMD_DOMAIN_ALERT",
	"HQMV2_MBOX_VF_CMD_NOTIFICATION",
	"HQMV2_MBOX_VF_CMD_IN_USE",
};

#define HQMV2_MBOX_CMD_TYPE(hdr) \
	(((struct hqmv2_mbox_req_hdr *)hdr)->type)
#define HQMV2_MBOX_CMD_STRING(hdr) \
	hqmv2_mbox_cmd_type_strings[HQMV2_MBOX_CMD_TYPE(hdr)]

enum hqmv2_mbox_status_type {
	HQMV2_MBOX_ST_SUCCESS,
	HQMV2_MBOX_ST_INVALID_CMD_TYPE,
	HQMV2_MBOX_ST_VERSION_MISMATCH,
	HQMV2_MBOX_ST_INVALID_OWNER_VF,
};

static const char hqmv2_mbox_status_type_strings[][128] = {
	"HQMV2_MBOX_ST_SUCCESS",
	"HQMV2_MBOX_ST_INVALID_CMD_TYPE",
	"HQMV2_MBOX_ST_VERSION_MISMATCH",
	"HQMV2_MBOX_ST_INVALID_OWNER_VF",
};

#define HQMV2_MBOX_ST_TYPE(hdr) \
	(((struct hqmv2_mbox_resp_hdr *)hdr)->status)
#define HQMV2_MBOX_ST_STRING(hdr) \
	hqmv2_mbox_status_type_strings[HQMV2_MBOX_ST_TYPE(hdr)]

/* This structure is always the first field in a request structure */
struct hqmv2_mbox_req_hdr {
	u32 type;
};

/* This structure is always the first field in a response structure */
struct hqmv2_mbox_resp_hdr {
	u32 status;
};

struct hqmv2_mbox_register_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 interface_version;
};

struct hqmv2_mbox_register_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 interface_version;
	u8 pf_id;
	u8 vf_id;
	u8 is_auxiliary_vf;
	u8 primary_vf_id;
	u32 padding;
};

struct hqmv2_mbox_unregister_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 padding;
};

struct hqmv2_mbox_unregister_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 padding;
};

struct hqmv2_mbox_get_num_resources_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 padding;
};

struct hqmv2_mbox_get_num_resources_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u16 num_sched_domains;
	u16 num_ldb_queues;
	u16 num_ldb_ports;
	u16 num_cos0_ldb_ports;
	u16 num_cos1_ldb_ports;
	u16 num_cos2_ldb_ports;
	u16 num_cos3_ldb_ports;
	u16 num_dir_ports;
	u32 num_atomic_inflights;
	u32 num_hist_list_entries;
	u32 max_contiguous_hist_list_entries;
	u16 num_ldb_credits;
	u16 num_dir_credits;
};

struct hqmv2_mbox_create_sched_domain_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 num_ldb_queues;
	u32 num_ldb_ports;
	u32 num_cos0_ldb_ports;
	u32 num_cos1_ldb_ports;
	u32 num_cos2_ldb_ports;
	u32 num_cos3_ldb_ports;
	u32 num_dir_ports;
	u32 num_atomic_inflights;
	u32 num_hist_list_entries;
	u32 num_ldb_credits;
	u32 num_dir_credits;
	u8 cos_strict;
	u8 padding0[3];
	u32 padding1;
};

struct hqmv2_mbox_create_sched_domain_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 id;
};

struct hqmv2_mbox_reset_sched_domain_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 id;
};

struct hqmv2_mbox_reset_sched_domain_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
};

struct hqmv2_mbox_create_ldb_queue_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 domain_id;
	u32 num_sequence_numbers;
	u32 num_qid_inflights;
	u32 num_atomic_inflights;
	u32 lock_id_comp_level;
	u32 depth_threshold;
	u32 padding;
};

struct hqmv2_mbox_create_ldb_queue_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 id;
};

struct hqmv2_mbox_create_dir_queue_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 depth_threshold;
};

struct hqmv2_mbox_create_dir_queue_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 id;
};

struct hqmv2_mbox_create_ldb_port_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 domain_id;
	u64 pop_count_address;
	u16 cq_depth;
	u16 cq_history_list_size;
	u8 cos_id;
	u8 cos_strict;
	u16 padding1;
	u64 cq_base_address;
};

struct hqmv2_mbox_create_ldb_port_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 id;
};

struct hqmv2_mbox_create_dir_port_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 domain_id;
	u64 pop_count_address;
	u64 cq_base_address;
	u16 cq_depth;
	u16 padding0;
	s32 queue_id;
};

struct hqmv2_mbox_create_dir_port_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 id;
};

struct hqmv2_mbox_enable_ldb_port_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 padding;
};

struct hqmv2_mbox_enable_ldb_port_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding;
};

struct hqmv2_mbox_disable_ldb_port_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 padding;
};

struct hqmv2_mbox_disable_ldb_port_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding;
};

struct hqmv2_mbox_enable_dir_port_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 padding;
};

struct hqmv2_mbox_enable_dir_port_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding;
};

struct hqmv2_mbox_disable_dir_port_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 padding;
};

struct hqmv2_mbox_disable_dir_port_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding;
};

struct hqmv2_mbox_ldb_port_owned_by_domain_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 padding;
};

struct hqmv2_mbox_ldb_port_owned_by_domain_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	s32 owned;
};

struct hqmv2_mbox_dir_port_owned_by_domain_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 padding;
};

struct hqmv2_mbox_dir_port_owned_by_domain_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	s32 owned;
};

struct hqmv2_mbox_map_qid_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 qid;
	u32 priority;
	u32 padding0;
};

struct hqmv2_mbox_map_qid_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 id;
};

struct hqmv2_mbox_unmap_qid_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 qid;
};

struct hqmv2_mbox_unmap_qid_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding;
};

struct hqmv2_mbox_start_domain_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 domain_id;
};

struct hqmv2_mbox_start_domain_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding;
};

struct hqmv2_mbox_enable_ldb_port_intr_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u16 port_id;
	u16 thresh;
	u16 vector;
	u16 owner_vf;
	u16 reserved[2];
};

struct hqmv2_mbox_enable_ldb_port_intr_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding;
};

struct hqmv2_mbox_enable_dir_port_intr_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u16 port_id;
	u16 thresh;
	u16 vector;
	u16 owner_vf;
	u16 reserved[2];
};

struct hqmv2_mbox_enable_dir_port_intr_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding;
};

struct hqmv2_mbox_arm_cq_intr_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 is_ldb;
};

struct hqmv2_mbox_arm_cq_intr_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 padding0;
};

/* The alert_id and aux_alert_data follows the format of the alerts defined in
 * hqmv2_types.h. The alert id contains an enum hqmv2_domain_alert_id value, and
 * the aux_alert_data value varies depending on the alert.
 */
struct hqmv2_mbox_vf_alert_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 domain_id;
	u32 alert_id;
	u32 aux_alert_data;
};

enum hqmv2_mbox_vf_notification_type {
	HQMV2_MBOX_VF_NOTIFICATION_PRE_RESET,
	HQMV2_MBOX_VF_NOTIFICATION_POST_RESET,

	/* NUM_HQMV2_MBOX_VF_NOTIFICATION_TYPES must be last */
	NUM_HQMV2_MBOX_VF_NOTIFICATION_TYPES,
};

struct hqmv2_mbox_vf_notification_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 notification;
};

struct hqmv2_mbox_vf_in_use_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 padding;
};

struct hqmv2_mbox_vf_in_use_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 in_use;
};

struct hqmv2_mbox_init_cq_sched_count_measure_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 duration_us;
};

struct hqmv2_mbox_init_cq_sched_count_measure_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
};

struct hqmv2_mbox_collect_cq_sched_count_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u16 cq_id;
	u16 is_ldb;
};

struct hqmv2_mbox_collect_cq_sched_count_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 elapsed;
	u32 padding;
	u64 cq_sched_count;
};

struct hqmv2_mbox_get_sn_allocation_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 group_id;
};

struct hqmv2_mbox_get_sn_allocation_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 num;
};

struct hqmv2_mbox_get_ldb_queue_depth_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 domain_id;
	u32 queue_id;
	u32 padding;
};

struct hqmv2_mbox_get_ldb_queue_depth_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 depth;
};

struct hqmv2_mbox_get_dir_queue_depth_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 domain_id;
	u32 queue_id;
	u32 padding;
};

struct hqmv2_mbox_get_dir_queue_depth_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 depth;
};

struct hqmv2_mbox_pending_port_unmaps_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 domain_id;
	u32 port_id;
	u32 padding;
};

struct hqmv2_mbox_pending_port_unmaps_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 error_code;
	u32 status;
	u32 num;
};

struct hqmv2_mbox_get_cos_bw_cmd_req {
	struct hqmv2_mbox_req_hdr hdr;
	u32 cos_id;
};

struct hqmv2_mbox_get_cos_bw_cmd_resp {
	struct hqmv2_mbox_resp_hdr hdr;
	u32 num;
};

#endif /* __HQMV2_BASE_HQMV2_MBOX_H */
