/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
 * Copyright(c) 2016-2020 Intel Corporation
 */

#ifndef __DLB_USER_H
#define __DLB_USER_H

#include <linux/types.h>

enum dlb_error {
	DLB_ST_SUCCESS = 0,
	DLB_ST_NAME_EXISTS,
	DLB_ST_DOMAIN_UNAVAILABLE,
	DLB_ST_LDB_PORTS_UNAVAILABLE,
	DLB_ST_DIR_PORTS_UNAVAILABLE,
	DLB_ST_LDB_QUEUES_UNAVAILABLE,
	DLB_ST_LDB_CREDITS_UNAVAILABLE,
	DLB_ST_DIR_CREDITS_UNAVAILABLE,
	DLB_ST_LDB_CREDIT_POOLS_UNAVAILABLE,
	DLB_ST_DIR_CREDIT_POOLS_UNAVAILABLE,
	DLB_ST_SEQUENCE_NUMBERS_UNAVAILABLE,
	DLB_ST_INVALID_DOMAIN_ID,
	DLB_ST_INVALID_QID_INFLIGHT_ALLOCATION,
	DLB_ST_ATOMIC_INFLIGHTS_UNAVAILABLE,
	DLB_ST_HIST_LIST_ENTRIES_UNAVAILABLE,
	DLB_ST_INVALID_LDB_CREDIT_POOL_ID,
	DLB_ST_INVALID_DIR_CREDIT_POOL_ID,
	DLB_ST_INVALID_POP_COUNT_VIRT_ADDR,
	DLB_ST_INVALID_LDB_QUEUE_ID,
	DLB_ST_INVALID_CQ_DEPTH,
	DLB_ST_INVALID_CQ_VIRT_ADDR,
	DLB_ST_INVALID_PORT_ID,
	DLB_ST_INVALID_QID,
	DLB_ST_INVALID_PRIORITY,
	DLB_ST_NO_QID_SLOTS_AVAILABLE,
	DLB_ST_QED_FREELIST_ENTRIES_UNAVAILABLE,
	DLB_ST_DQED_FREELIST_ENTRIES_UNAVAILABLE,
	DLB_ST_INVALID_DIR_QUEUE_ID,
	DLB_ST_DIR_QUEUES_UNAVAILABLE,
	DLB_ST_INVALID_LDB_CREDIT_LOW_WATERMARK,
	DLB_ST_INVALID_LDB_CREDIT_QUANTUM,
	DLB_ST_INVALID_DIR_CREDIT_LOW_WATERMARK,
	DLB_ST_INVALID_DIR_CREDIT_QUANTUM,
	DLB_ST_DOMAIN_NOT_CONFIGURED,
	DLB_ST_PID_ALREADY_ATTACHED,
	DLB_ST_PID_NOT_ATTACHED,
	DLB_ST_INTERNAL_ERROR,
	DLB_ST_DOMAIN_IN_USE,
	DLB_ST_IOMMU_MAPPING_ERROR,
	DLB_ST_FAIL_TO_PIN_MEMORY_PAGE,
	DLB_ST_UNABLE_TO_PIN_POPCOUNT_PAGES,
	DLB_ST_UNABLE_TO_PIN_CQ_PAGES,
	DLB_ST_DISCONTIGUOUS_CQ_MEMORY,
	DLB_ST_DISCONTIGUOUS_POP_COUNT_MEMORY,
	DLB_ST_DOMAIN_STARTED,
	DLB_ST_LARGE_POOL_NOT_SPECIFIED,
	DLB_ST_SMALL_POOL_NOT_SPECIFIED,
	DLB_ST_NEITHER_POOL_SPECIFIED,
	DLB_ST_DOMAIN_NOT_STARTED,
	DLB_ST_INVALID_MEASUREMENT_DURATION,
	DLB_ST_INVALID_PERF_METRIC_GROUP_ID,
	DLB_ST_LDB_PORT_REQUIRED_FOR_LDB_QUEUES,
	DLB_ST_DOMAIN_RESET_FAILED,
	DLB_ST_MBOX_ERROR,
	DLB_ST_INVALID_HIST_LIST_DEPTH,
	DLB_ST_NO_MEMORY,
};

static const char dlb_error_strings[][128] = {
	"DLB_ST_SUCCESS",
	"DLB_ST_NAME_EXISTS",
	"DLB_ST_DOMAIN_UNAVAILABLE",
	"DLB_ST_LDB_PORTS_UNAVAILABLE",
	"DLB_ST_DIR_PORTS_UNAVAILABLE",
	"DLB_ST_LDB_QUEUES_UNAVAILABLE",
	"DLB_ST_LDB_CREDITS_UNAVAILABLE",
	"DLB_ST_DIR_CREDITS_UNAVAILABLE",
	"DLB_ST_LDB_CREDIT_POOLS_UNAVAILABLE",
	"DLB_ST_DIR_CREDIT_POOLS_UNAVAILABLE",
	"DLB_ST_SEQUENCE_NUMBERS_UNAVAILABLE",
	"DLB_ST_INVALID_DOMAIN_ID",
	"DLB_ST_INVALID_QID_INFLIGHT_ALLOCATION",
	"DLB_ST_ATOMIC_INFLIGHTS_UNAVAILABLE",
	"DLB_ST_HIST_LIST_ENTRIES_UNAVAILABLE",
	"DLB_ST_INVALID_LDB_CREDIT_POOL_ID",
	"DLB_ST_INVALID_DIR_CREDIT_POOL_ID",
	"DLB_ST_INVALID_POP_COUNT_VIRT_ADDR",
	"DLB_ST_INVALID_LDB_QUEUE_ID",
	"DLB_ST_INVALID_CQ_DEPTH",
	"DLB_ST_INVALID_CQ_VIRT_ADDR",
	"DLB_ST_INVALID_PORT_ID",
	"DLB_ST_INVALID_QID",
	"DLB_ST_INVALID_PRIORITY",
	"DLB_ST_NO_QID_SLOTS_AVAILABLE",
	"DLB_ST_QED_FREELIST_ENTRIES_UNAVAILABLE",
	"DLB_ST_DQED_FREELIST_ENTRIES_UNAVAILABLE",
	"DLB_ST_INVALID_DIR_QUEUE_ID",
	"DLB_ST_DIR_QUEUES_UNAVAILABLE",
	"DLB_ST_INVALID_LDB_CREDIT_LOW_WATERMARK",
	"DLB_ST_INVALID_LDB_CREDIT_QUANTUM",
	"DLB_ST_INVALID_DIR_CREDIT_LOW_WATERMARK",
	"DLB_ST_INVALID_DIR_CREDIT_QUANTUM",
	"DLB_ST_DOMAIN_NOT_CONFIGURED",
	"DLB_ST_PID_ALREADY_ATTACHED",
	"DLB_ST_PID_NOT_ATTACHED",
	"DLB_ST_INTERNAL_ERROR",
	"DLB_ST_DOMAIN_IN_USE",
	"DLB_ST_IOMMU_MAPPING_ERROR",
	"DLB_ST_FAIL_TO_PIN_MEMORY_PAGE",
	"DLB_ST_UNABLE_TO_PIN_POPCOUNT_PAGES",
	"DLB_ST_UNABLE_TO_PIN_CQ_PAGES",
	"DLB_ST_DISCONTIGUOUS_CQ_MEMORY",
	"DLB_ST_DISCONTIGUOUS_POP_COUNT_MEMORY",
	"DLB_ST_DOMAIN_STARTED",
	"DLB_ST_LARGE_POOL_NOT_SPECIFIED",
	"DLB_ST_SMALL_POOL_NOT_SPECIFIED",
	"DLB_ST_NEITHER_POOL_SPECIFIED",
	"DLB_ST_DOMAIN_NOT_STARTED",
	"DLB_ST_INVALID_MEASUREMENT_DURATION",
	"DLB_ST_INVALID_PERF_METRIC_GROUP_ID",
	"DLB_ST_LDB_PORT_REQUIRED_FOR_LDB_QUEUES",
	"DLB_ST_DOMAIN_RESET_FAILED",
	"DLB_ST_MBOX_ERROR",
	"DLB_ST_INVALID_HIST_LIST_DEPTH",
	"DLB_ST_NO_MEMORY",
};

struct dlb_cmd_response {
	__u32 status; /* Interpret using enum dlb_error */
	__u32 id;
};

/******************************/
/* 'dlb' device file commands */
/******************************/

#define DLB_DEVICE_VERSION(x) (((x) >> 8) & 0xFF)
#define DLB_DEVICE_REVISION(x) ((x) & 0xFF)

enum dlb_revisions {
	DLB_REV_A0 = 0,
	DLB_REV_A1 = 1,
	DLB_REV_A2 = 2,
	DLB_REV_A3 = 3,
	DLB_REV_B0 = 4,
};

/*
 * DLB_CMD_GET_DEVICE_VERSION: Query the DLB device version.
 *
 *	This ioctl interface is the same in all driver versions and is always
 *	the first ioctl.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id[7:0]: Device revision.
 *	response.id[15:8]: Device version.
 */

struct dlb_get_device_version_args {
	/* Output parameters */
	__u64 response;
};

#define DLB_VERSION_MAJOR_NUMBER 10
#define DLB_VERSION_MINOR_NUMBER 7
#define DLB_VERSION_REVISION_NUMBER 8
#define DLB_VERSION (DLB_VERSION_MAJOR_NUMBER << 24 | \
		     DLB_VERSION_MINOR_NUMBER << 16 | \
		     DLB_VERSION_REVISION_NUMBER)

#define DLB_VERSION_GET_MAJOR_NUMBER(x) (((x) >> 24) & 0xFF)
#define DLB_VERSION_GET_MINOR_NUMBER(x) (((x) >> 16) & 0xFF)
#define DLB_VERSION_GET_REVISION_NUMBER(x) ((x) & 0xFFFF)

static inline __u8 dlb_version_incompatible(__u32 version)
{
	__u8 inc;

	inc = DLB_VERSION_GET_MAJOR_NUMBER(version) != DLB_VERSION_MAJOR_NUMBER;
	inc |= (int)DLB_VERSION_GET_MINOR_NUMBER(version) <
		DLB_VERSION_MINOR_NUMBER;

	return inc;
}

/*
 * DLB_CMD_GET_DRIVER_VERSION: Query the DLB driver version. The major number
 *	is changed when there is an ABI-breaking change, the minor number is
 *	changed if the API is changed in a backwards-compatible way, and the
 *	revision number is changed for fixes that don't affect the API.
 *
 *	If the kernel driver's API version major number and the header's
 *	DLB_VERSION_MAJOR_NUMBER differ, the two are incompatible, or if the
 *	major numbers match but the kernel driver's minor number is less than
 *	the header file's, they are incompatible. The DLB_VERSION_INCOMPATIBLE
 *	macro should be used to check for compatibility.
 *
 *	This ioctl interface is the same in all driver versions. Applications
 *	should check the driver version before performing any other ioctl
 *	operations.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Driver API version. Use the DLB_VERSION_GET_MAJOR_NUMBER,
 *		DLB_VERSION_GET_MINOR_NUMBER, and
 *		DLB_VERSION_GET_REVISION_NUMBER macros to interpret the field.
 */

struct dlb_get_driver_version_args {
	/* Output parameters */
	__u64 response;
};

/*
 * DLB_CMD_CREATE_SCHED_DOMAIN: Create a DLB scheduling domain and reserve the
 *	resources (queues, ports, etc.) that it contains.
 *
 * Input parameters:
 * - num_ldb_queues: Number of load-balanced queues.
 * - num_ldb_ports: Number of load-balanced ports.
 * - num_dir_ports: Number of directed ports. A directed port has one directed
 *	queue, so no num_dir_queues argument is necessary.
 * - num_atomic_inflights: This specifies the amount of temporary atomic QE
 *	storage for the domain. This storage is divided among the domain's
 *	load-balanced queues that are configured for atomic scheduling.
 * - num_hist_list_entries: Amount of history list storage. This is divided
 *	among the domain's CQs.
 * - num_ldb_credits: Amount of load-balanced QE storage (QED). QEs occupy this
 *	space until they are scheduled to a load-balanced CQ. One credit
 *	represents the storage for one QE.
 * - num_dir_credits: Amount of directed QE storage (DQED). QEs occupy this
 *	space until they are scheduled to a directed CQ. One credit represents
 *	the storage for one QE.
 * - num_ldb_credit_pools: Number of pools into which the load-balanced credits
 *	are placed.
 * - num_dir_credit_pools: Number of pools into which the directed credits are
 *	placed.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: domain ID.
 */
struct dlb_create_sched_domain_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 num_ldb_queues;
	__u32 num_ldb_ports;
	__u32 num_dir_ports;
	__u32 num_atomic_inflights;
	__u32 num_hist_list_entries;
	__u32 num_ldb_credits;
	__u32 num_dir_credits;
	__u32 num_ldb_credit_pools;
	__u32 num_dir_credit_pools;
};

/*
 * DLB_CMD_GET_NUM_RESOURCES: Return the number of available resources
 *	(queues, ports, etc.) that this device owns.
 *
 * Output parameters:
 * - num_domains: Number of available scheduling domains.
 * - num_ldb_queues: Number of available load-balanced queues.
 * - num_ldb_ports: Number of available load-balanced ports.
 * - num_dir_ports: Number of available directed ports. There is one directed
 *	queue for every directed port.
 * - num_atomic_inflights: Amount of available temporary atomic QE storage.
 * - max_contiguous_atomic_inflights: When a domain is created, the temporary
 *	atomic QE storage is allocated in a contiguous chunk. This return value
 *	is the longest available contiguous range of atomic QE storage.
 * - num_hist_list_entries: Amount of history list storage.
 * - max_contiguous_hist_list_entries: History list storage is allocated in
 *	a contiguous chunk, and this return value is the longest available
 *	contiguous range of history list entries.
 * - num_ldb_credits: Amount of available load-balanced QE storage.
 * - max_contiguous_ldb_credits: QED storage is allocated in a contiguous
 *	chunk, and this return value is the longest available contiguous range
 *	of load-balanced credit storage.
 * - num_dir_credits: Amount of available directed QE storage.
 * - max_contiguous_dir_credits: DQED storage is allocated in a contiguous
 *	chunk, and this return value is the longest available contiguous range
 *	of directed credit storage.
 * - num_ldb_credit_pools: Number of available load-balanced credit pools.
 * - num_dir_credit_pools: Number of available directed credit pools.
 * - padding0: Reserved for future use.
 */
struct dlb_get_num_resources_args {
	/* Output parameters */
	__u32 num_sched_domains;
	__u32 num_ldb_queues;
	__u32 num_ldb_ports;
	__u32 num_dir_ports;
	__u32 num_atomic_inflights;
	__u32 max_contiguous_atomic_inflights;
	__u32 num_hist_list_entries;
	__u32 max_contiguous_hist_list_entries;
	__u32 num_ldb_credits;
	__u32 max_contiguous_ldb_credits;
	__u32 num_dir_credits;
	__u32 max_contiguous_dir_credits;
	__u32 num_ldb_credit_pools;
	__u32 num_dir_credit_pools;
	__u32 padding0;
};

/*
 * DLB_CMD_SET_SN_ALLOCATION: Configure a sequence number group
 *
 * Input parameters:
 * - group: Sequence number group ID.
 * - num: Number of sequence numbers per queue.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb_set_sn_allocation_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 group;
	__u32 num;
};

/*
 * DLB_CMD_GET_SN_ALLOCATION: Get a sequence number group's configuration
 *
 * Input parameters:
 * - group: Sequence number group ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Specified group's number of sequence numbers per queue.
 */
struct dlb_get_sn_allocation_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 group;
	__u32 padding0;
};

/*
 * DLB_CMD_QUERY_CQ_POLL_MODE: Query the CQ poll mode the kernel driver is using
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: CQ poll mode (see enum dlb_cq_poll_modes).
 */
struct dlb_query_cq_poll_mode_args {
	/* Output parameters */
	__u64 response;
};

enum dlb_cq_poll_modes {
	DLB_CQ_POLL_MODE_STD,
	DLB_CQ_POLL_MODE_SPARSE,

	/* NUM_DLB_CQ_POLL_MODE must be last */
	NUM_DLB_CQ_POLL_MODE,
};

/*
 * DLB_CMD_GET_SN_OCCUPANCY: Get a sequence number group's occupancy
 *
 * Each sequence number group has one or more slots, depending on its
 * configuration. I.e.:
 * - If configured for 1024 sequence numbers per queue, the group has 1 slot
 * - If configured for 512 sequence numbers per queue, the group has 2 slots
 *   ...
 * - If configured for 32 sequence numbers per queue, the group has 32 slots
 *
 * This ioctl returns the group's number of in-use slots. If its occupancy is
 * 0, the group's sequence number allocation can be reconfigured.
 *
 * Input parameters:
 * - group: Sequence number group ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Specified group's number of used slots.
 */
struct dlb_get_sn_occupancy_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 group;
	__u32 padding0;
};

enum dlb_user_interface_commands {
	DLB_CMD_GET_DEVICE_VERSION,
	DLB_CMD_CREATE_SCHED_DOMAIN,
	DLB_CMD_GET_NUM_RESOURCES,
	DLB_CMD_GET_DRIVER_VERSION,
	DLB_CMD_SET_SN_ALLOCATION,
	DLB_CMD_GET_SN_ALLOCATION,
	DLB_CMD_QUERY_CQ_POLL_MODE,
	DLB_CMD_GET_SN_OCCUPANCY,

	/* NUM_DLB_CMD must be last */
	NUM_DLB_CMD,
};

/*******************************/
/* 'domain' device file alerts */
/*******************************/

/* Scheduling domain device files can be read to receive domain-specific
 * notifications, for alerts such as hardware errors.
 *
 * Each alert is encoded in a 16B message. The first 8B contains the alert ID,
 * and the second 8B is optional and contains additional information.
 * Applications should cast read data to a struct dlb_domain_alert, and
 * interpret the struct's alert_id according to dlb_domain_alert_id. The read
 * length must be 16B, or the function will return -EINVAL.
 *
 * Reads are destructive, and in the case of multiple file descriptors for the
 * same domain device file, an alert will be read by only one of the file
 * descriptors.
 *
 * The driver stores alerts in a fixed-size alert ring until they are read. If
 * the alert ring fills completely, subsequent alerts will be dropped. It is
 * recommended that DLB applications dedicate a thread to perform blocking
 * reads on the device file.
 */
enum dlb_domain_alert_id {
	/* A destination domain queue that this domain connected to has
	 * unregistered, and can no longer be sent to. The aux alert data
	 * contains the queue ID.
	 */
	DLB_DOMAIN_ALERT_REMOTE_QUEUE_UNREGISTER,
	/* A producer port in this domain attempted to send a QE without a
	 * credit. aux_alert_data[7:0] contains the port ID, and
	 * aux_alert_data[15:8] contains a flag indicating whether the port is
	 * load-balanced (1) or directed (0).
	 */
	DLB_DOMAIN_ALERT_PP_OUT_OF_CREDITS,
	/* Software issued an illegal enqueue for a port in this domain. An
	 * illegal enqueue could be:
	 * - Illegal (excess) completion
	 * - Illegal fragment
	 * - Illegal enqueue command
	 * aux_alert_data[7:0] contains the port ID, and aux_alert_data[15:8]
	 * contains a flag indicating whether the port is load-balanced (1) or
	 * directed (0).
	 */
	DLB_DOMAIN_ALERT_PP_ILLEGAL_ENQ,
	/* Software issued excess CQ token pops for a port in this domain.
	 * aux_alert_data[7:0] contains the port ID, and aux_alert_data[15:8]
	 * contains a flag indicating whether the port is load-balanced (1) or
	 * directed (0).
	 */
	DLB_DOMAIN_ALERT_PP_EXCESS_TOKEN_POPS,
	/* A enqueue contained either an invalid command encoding or a REL,
	 * REL_T, RLS, FWD, FWD_T, FRAG, or FRAG_T from a directed port.
	 *
	 * aux_alert_data[7:0] contains the port ID, and aux_alert_data[15:8]
	 * contains a flag indicating whether the port is load-balanced (1) or
	 * directed (0).
	 */
	DLB_DOMAIN_ALERT_ILLEGAL_HCW,
	/* The QID must be valid and less than 128.
	 *
	 * aux_alert_data[7:0] contains the port ID, and aux_alert_data[15:8]
	 * contains a flag indicating whether the port is load-balanced (1) or
	 * directed (0).
	 */
	DLB_DOMAIN_ALERT_ILLEGAL_QID,
	/* An enqueue went to a disabled QID.
	 *
	 * aux_alert_data[7:0] contains the port ID, and aux_alert_data[15:8]
	 * contains a flag indicating whether the port is load-balanced (1) or
	 * directed (0).
	 */
	DLB_DOMAIN_ALERT_DISABLED_QID,
	/* The device containing this domain was reset. All applications using
	 * the device need to exit for the driver to complete the reset
	 * procedure.
	 *
	 * aux_alert_data doesn't contain any information for this alert.
	 */
	DLB_DOMAIN_ALERT_DEVICE_RESET,
	/* User-space has enqueued an alert.
	 *
	 * aux_alert_data contains user-provided data.
	 */
	DLB_DOMAIN_ALERT_USER,

	/* Number of DLB domain alerts */
	NUM_DLB_DOMAIN_ALERTS
};

static const char dlb_domain_alert_strings[][128] = {
	"DLB_DOMAIN_ALERT_REMOTE_QUEUE_UNREGISTER",
	"DLB_DOMAIN_ALERT_PP_OUT_OF_CREDITS",
	"DLB_DOMAIN_ALERT_PP_ILLEGAL_ENQ",
	"DLB_DOMAIN_ALERT_PP_EXCESS_TOKEN_POPS",
	"DLB_DOMAIN_ALERT_ILLEGAL_HCW",
	"DLB_DOMAIN_ALERT_ILLEGAL_QID",
	"DLB_DOMAIN_ALERT_DISABLED_QID",
	"DLB_DOMAIN_ALERT_DEVICE_RESET",
	"DLB_DOMAIN_ALERT_USER",
};

struct dlb_domain_alert {
	__u64 alert_id;
	__u64 aux_alert_data;
};

/*********************************/
/* 'domain' device file commands */
/*********************************/

/*
 * DLB_DOMAIN_CMD_CREATE_LDB_POOL: Configure a load-balanced credit pool.
 * Input parameters:
 * - num_ldb_credits: Number of load-balanced credits (QED space) for this
 *	pool.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: pool ID.
 */
struct dlb_create_ldb_pool_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 num_ldb_credits;
	__u32 padding0;
};

/*
 * DLB_DOMAIN_CMD_CREATE_DIR_POOL: Configure a directed credit pool.
 * Input parameters:
 * - num_dir_credits: Number of directed credits (DQED space) for this pool.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Pool ID.
 */
struct dlb_create_dir_pool_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 num_dir_credits;
	__u32 padding0;
};

/*
 * DLB_DOMAIN_CMD_CREATE_LDB_QUEUE: Configure a load-balanced queue.
 * Input parameters:
 * - num_atomic_inflights: This specifies the amount of temporary atomic QE
 *	storage for this queue. If zero, the queue will not support atomic
 *	scheduling.
 * - num_sequence_numbers: This specifies the number of sequence numbers used
 *	by this queue. If zero, the queue will not support ordered scheduling.
 *	If non-zero, the queue will not support unordered scheduling.
 * - num_qid_inflights: The maximum number of QEs that can be inflight
 *	(scheduled to a CQ but not completed) at any time. If
 *	num_sequence_numbers is non-zero, num_qid_inflights must be set equal
 *	to num_sequence_numbers.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Queue ID.
 */
struct dlb_create_ldb_queue_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 num_sequence_numbers;
	__u32 num_qid_inflights;
	__u32 num_atomic_inflights;
	__u32 padding0;
};

/*
 * DLB_DOMAIN_CMD_CREATE_DIR_QUEUE: Configure a directed queue.
 * Input parameters:
 * - port_id: Port ID. If the corresponding directed port is already created,
 *	specify its ID here. Else this argument must be 0xFFFFFFFF to indicate
 *	that the queue is being created before the port.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Queue ID.
 */
struct dlb_create_dir_queue_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__s32 port_id;
	__u32 padding0;
};

/*
 * DLB_DOMAIN_CMD_CREATE_LDB_PORT: Configure a load-balanced port.
 * Input parameters:
 * - ldb_credit_pool_id: Load-balanced credit pool this port will belong to.
 * - dir_credit_pool_id: Directed credit pool this port will belong to.
 * - ldb_credit_high_watermark: Number of load-balanced credits from the pool
 *	that this port will own.
 *
 *	If this port's scheduling domain doesn't have any load-balanced queues,
 *	this argument is ignored and the port is given no load-balanced
 *	credits.
 * - dir_credit_high_watermark: Number of directed credits from the pool that
 *	this port will own.
 *
 *	If this port's scheduling domain doesn't have any directed queues,
 *	this argument is ignored and the port is given no directed credits.
 * - ldb_credit_low_watermark: Load-balanced credit low watermark. When the
 *	port's credits reach this watermark, they become eligible to be
 *	refilled by the DLB as credits until the high watermark
 *	(num_ldb_credits) is reached.
 *
 *	If this port's scheduling domain doesn't have any load-balanced queues,
 *	this argument is ignored and the port is given no load-balanced
 *	credits.
 * - dir_credit_low_watermark: Directed credit low watermark. When the port's
 *	credits reach this watermark, they become eligible to be refilled by
 *	the DLB as credits until the high watermark (num_dir_credits) is
 *	reached.
 *
 *	If this port's scheduling domain doesn't have any directed queues,
 *	this argument is ignored and the port is given no directed credits.
 * - ldb_credit_quantum: Number of load-balanced credits for the DLB to refill
 *	per refill operation.
 *
 *	If this port's scheduling domain doesn't have any load-balanced queues,
 *	this argument is ignored and the port is given no load-balanced
 *	credits.
 * - dir_credit_quantum: Number of directed credits for the DLB to refill per
 *	refill operation.
 *
 *	If this port's scheduling domain doesn't have any directed queues,
 *	this argument is ignored and the port is given no directed credits.
 * - padding0: Reserved for future use.
 * - cq_depth: Depth of the port's CQ. Must be a power-of-two between 8 and
 *	1024, inclusive.
 * - cq_depth_threshold: CQ depth interrupt threshold. A value of N means that
 *	the CQ interrupt won't fire until there are N or more outstanding CQ
 *	tokens.
 * - cq_history_list_size: Number of history list entries. This must be greater
 *	than or equal to cq_depth.
 * - padding1: Reserved for future use.
 * - padding2: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: port ID.
 */
struct dlb_create_ldb_port_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 ldb_credit_pool_id;
	__u32 dir_credit_pool_id;
	__u16 ldb_credit_high_watermark;
	__u16 ldb_credit_low_watermark;
	__u16 ldb_credit_quantum;
	__u16 dir_credit_high_watermark;
	__u16 dir_credit_low_watermark;
	__u16 dir_credit_quantum;
	__u16 padding0;
	__u16 cq_depth;
	__u16 cq_depth_threshold;
	__u16 cq_history_list_size;
	__u32 padding1;
};

/*
 * DLB_DOMAIN_CMD_CREATE_DIR_PORT: Configure a directed port.
 * Input parameters:
 * - ldb_credit_pool_id: Load-balanced credit pool this port will belong to.
 * - dir_credit_pool_id: Directed credit pool this port will belong to.
 * - ldb_credit_high_watermark: Number of load-balanced credits from the pool
 *	that this port will own.
 *
 *	If this port's scheduling domain doesn't have any load-balanced queues,
 *	this argument is ignored and the port is given no load-balanced
 *	credits.
 * - dir_credit_high_watermark: Number of directed credits from the pool that
 *	this port will own.
 * - ldb_credit_low_watermark: Load-balanced credit low watermark. When the
 *	port's credits reach this watermark, they become eligible to be
 *	refilled by the DLB as credits until the high watermark
 *	(num_ldb_credits) is reached.
 *
 *	If this port's scheduling domain doesn't have any load-balanced queues,
 *	this argument is ignored and the port is given no load-balanced
 *	credits.
 * - dir_credit_low_watermark: Directed credit low watermark. When the port's
 *	credits reach this watermark, they become eligible to be refilled by
 *	the DLB as credits until the high watermark (num_dir_credits) is
 *	reached.
 * - ldb_credit_quantum: Number of load-balanced credits for the DLB to refill
 *	per refill operation.
 *
 *	If this port's scheduling domain doesn't have any load-balanced queues,
 *	this argument is ignored and the port is given no load-balanced
 *	credits.
 * - dir_credit_quantum: Number of directed credits for the DLB to refill per
 *	refill operation.
 * - cq_depth: Depth of the port's CQ. Must be a power-of-two between 8 and
 *	1024, inclusive.
 * - cq_depth_threshold: CQ depth interrupt threshold. A value of N means that
 *	the CQ interrupt won't fire until there are N or more outstanding CQ
 *	tokens.
 * - qid: Queue ID. If the corresponding directed queue is already created,
 *	specify its ID here. Else this argument must be 0xFFFFFFFF to indicate
 *	that the port is being created before the queue.
 * - padding1: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Port ID.
 */
struct dlb_create_dir_port_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 ldb_credit_pool_id;
	__u32 dir_credit_pool_id;
	__u16 ldb_credit_high_watermark;
	__u16 ldb_credit_low_watermark;
	__u16 ldb_credit_quantum;
	__u16 dir_credit_high_watermark;
	__u16 dir_credit_low_watermark;
	__u16 dir_credit_quantum;
	__u16 cq_depth;
	__u16 cq_depth_threshold;
	__s32 queue_id;
	__u32 padding1;
};

/*
 * DLB_DOMAIN_CMD_START_DOMAIN: Mark the end of the domain configuration. This
 *	must be called before passing QEs into the device, and no configuration
 *	ioctls can be issued once the domain has started. Sending QEs into the
 *	device before calling this ioctl will result in undefined behavior.
 * Input parameters:
 * - (None)
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb_start_domain_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
};

/*
 * DLB_DOMAIN_CMD_MAP_QID: Map a load-balanced queue to a load-balanced port.
 * Input parameters:
 * - port_id: Load-balanced port ID.
 * - qid: Load-balanced queue ID.
 * - priority: Queue->port service priority.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb_map_qid_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u32 qid;
	__u32 priority;
	__u32 padding0;
};

/*
 * DLB_DOMAIN_CMD_UNMAP_QID: Unmap a load-balanced queue to a load-balanced
 *	port.
 * Input parameters:
 * - port_id: Load-balanced port ID.
 * - qid: Load-balanced queue ID.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb_unmap_qid_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u32 qid;
};

/*
 * DLB_DOMAIN_CMD_ENABLE_LDB_PORT: Enable scheduling to a load-balanced port.
 * Input parameters:
 * - port_id: Load-balanced port ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb_enable_ldb_port_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u32 padding0;
};

/*
 * DLB_DOMAIN_CMD_ENABLE_DIR_PORT: Enable scheduling to a directed port.
 * Input parameters:
 * - port_id: Directed port ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb_enable_dir_port_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
};

/*
 * DLB_DOMAIN_CMD_DISABLE_LDB_PORT: Disable scheduling to a load-balanced port.
 * Input parameters:
 * - port_id: Load-balanced port ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb_disable_ldb_port_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u32 padding0;
};

/*
 * DLB_DOMAIN_CMD_DISABLE_DIR_PORT: Disable scheduling to a directed port.
 * Input parameters:
 * - port_id: Directed port ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *
 */
struct dlb_disable_dir_port_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u32 padding0;
};

/*
 * DLB_DOMAIN_CMD_BLOCK_ON_CQ_INTERRUPT: Block on a CQ interrupt until a QE
 *	arrives for the specified port. If a QE is already present, the ioctl
 *	will immediately return.
 *
 *	Note: Only one thread can block on a CQ's interrupt at a time. Doing
 *	otherwise can result in hung threads.
 *
 * Input parameters:
 * - port_id: Port ID.
 * - is_ldb: True if the port is load-balanced, false otherwise.
 * - arm: Tell the driver to arm the interrupt.
 * - cq_gen: Current CQ generation bit.
 * - padding0: Reserved for future use.
 * - cq_va: VA of the CQ entry where the next QE will be placed.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb_block_on_cq_interrupt_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u8 is_ldb;
	__u8 arm;
	__u8 cq_gen;
	__u8 padding0;
	__u64 cq_va;
};

/*
 * DLB_DOMAIN_CMD_ENQUEUE_DOMAIN_ALERT: Enqueue a domain alert that will be
 *	read by one reader thread.
 *
 * Input parameters:
 * - aux_alert_data: user-defined auxiliary data.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb_enqueue_domain_alert_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u64 aux_alert_data;
};

/*
 * DLB_DOMAIN_CMD_GET_LDB_QUEUE_DEPTH: Get a load-balanced queue's depth.
 * Input parameters:
 * - queue_id: The load-balanced queue ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: queue depth.
 */
struct dlb_get_ldb_queue_depth_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 queue_id;
	__u32 padding0;
};

/*
 * DLB_DOMAIN_CMD_GET_DIR_QUEUE_DEPTH: Get a directed queue's depth.
 * Input parameters:
 * - queue_id: The directed queue ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: queue depth.
 */
struct dlb_get_dir_queue_depth_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 queue_id;
	__u32 padding0;
};

/*
 * DLB_DOMAIN_CMD_PENDING_PORT_UNMAPS: Get number of queue unmap operations in
 *	progress for a load-balanced port.
 *
 *	Note: This is a snapshot; the number of unmap operations in progress
 *	is subject to change at any time.
 *
 * Input parameters:
 * - port_id: Load-balanced port ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: number of unmaps in progress.
 */
struct dlb_pending_port_unmaps_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u32 padding0;
};

enum dlb_domain_user_interface_commands {
	DLB_DOMAIN_CMD_CREATE_LDB_POOL,
	DLB_DOMAIN_CMD_CREATE_DIR_POOL,
	DLB_DOMAIN_CMD_CREATE_LDB_QUEUE,
	DLB_DOMAIN_CMD_CREATE_DIR_QUEUE,
	DLB_DOMAIN_CMD_CREATE_LDB_PORT,
	DLB_DOMAIN_CMD_CREATE_DIR_PORT,
	DLB_DOMAIN_CMD_START_DOMAIN,
	DLB_DOMAIN_CMD_MAP_QID,
	DLB_DOMAIN_CMD_UNMAP_QID,
	DLB_DOMAIN_CMD_ENABLE_LDB_PORT,
	DLB_DOMAIN_CMD_ENABLE_DIR_PORT,
	DLB_DOMAIN_CMD_DISABLE_LDB_PORT,
	DLB_DOMAIN_CMD_DISABLE_DIR_PORT,
	DLB_DOMAIN_CMD_BLOCK_ON_CQ_INTERRUPT,
	DLB_DOMAIN_CMD_ENQUEUE_DOMAIN_ALERT,
	DLB_DOMAIN_CMD_GET_LDB_QUEUE_DEPTH,
	DLB_DOMAIN_CMD_GET_DIR_QUEUE_DEPTH,
	DLB_DOMAIN_CMD_PENDING_PORT_UNMAPS,

	/* NUM_DLB_DOMAIN_CMD must be last */
	NUM_DLB_DOMAIN_CMD,
};

/*
 * Base addresses for memory mapping the consumer queue (CQ) and popcount (PC)
 * memory space, and producer port (PP) MMIO space. The CQ, PC, and PP
 * addresses are per-port. Every address is page-separated (e.g. LDB PP 0 is at
 * 0x2100000 and LDB PP 1 is at 0x2101000).
 */
#define DLB_LDB_CQ_BASE 0x3000000
#define DLB_LDB_CQ_MAX_SIZE 65536
#define DLB_LDB_CQ_OFFS(id) (DLB_LDB_CQ_BASE + (id) * DLB_LDB_CQ_MAX_SIZE)

#define DLB_DIR_CQ_BASE 0x3800000
#define DLB_DIR_CQ_MAX_SIZE 65536
#define DLB_DIR_CQ_OFFS(id) (DLB_DIR_CQ_BASE + (id) * DLB_DIR_CQ_MAX_SIZE)

#define DLB_LDB_PC_BASE 0x2300000
#define DLB_LDB_PC_MAX_SIZE 4096
#define DLB_LDB_PC_OFFS(id) (DLB_LDB_PC_BASE + (id) * DLB_LDB_PC_MAX_SIZE)

#define DLB_DIR_PC_BASE 0x2200000
#define DLB_DIR_PC_MAX_SIZE 4096
#define DLB_DIR_PC_OFFS(id) (DLB_DIR_PC_BASE + (id) * DLB_DIR_PC_MAX_SIZE)

#define DLB_LDB_PP_BASE 0x2100000
#define DLB_LDB_PP_MAX_SIZE 4096
#define DLB_LDB_PP_OFFS(id) (DLB_LDB_PP_BASE + (id) * DLB_LDB_PP_MAX_SIZE)

#define DLB_DIR_PP_BASE 0x2000000
#define DLB_DIR_PP_MAX_SIZE 4096
#define DLB_DIR_PP_OFFS(id) (DLB_DIR_PP_BASE + (id) * DLB_DIR_PP_MAX_SIZE)

/*******************/
/* dlb ioctl codes */
/*******************/

#define DLB_IOC_MAGIC  'h'

#define DLB_IOC_GET_DEVICE_VERSION				\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_CMD_GET_DEVICE_VERSION,		\
		      struct dlb_get_driver_version_args)
#define DLB_IOC_CREATE_SCHED_DOMAIN				\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_CMD_CREATE_SCHED_DOMAIN,		\
		      struct dlb_create_sched_domain_args)
#define DLB_IOC_GET_NUM_RESOURCES				\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_CMD_GET_NUM_RESOURCES,		\
		      struct dlb_get_num_resources_args)
#define DLB_IOC_GET_DRIVER_VERSION				\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_CMD_GET_DRIVER_VERSION,		\
		      struct dlb_get_driver_version_args)
#define DLB_IOC_SET_SN_ALLOCATION				\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_CMD_SET_SN_ALLOCATION,		\
		      struct dlb_set_sn_allocation_args)
#define DLB_IOC_GET_SN_ALLOCATION				\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_CMD_GET_SN_ALLOCATION,		\
		      struct dlb_get_sn_allocation_args)
#define DLB_IOC_QUERY_CQ_POLL_MODE				\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_CMD_QUERY_CQ_POLL_MODE,		\
		      struct dlb_query_cq_poll_mode_args)
#define DLB_IOC_GET_SN_OCCUPANCY				\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_CMD_GET_SN_OCCUPANCY,			\
		      struct dlb_get_sn_occupancy_args)
#define DLB_IOC_CREATE_LDB_POOL					\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_DOMAIN_CMD_CREATE_LDB_POOL,		\
		      struct dlb_create_ldb_pool_args)
#define DLB_IOC_CREATE_DIR_POOL					\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_DOMAIN_CMD_CREATE_DIR_POOL,		\
		      struct dlb_create_dir_pool_args)
#define DLB_IOC_CREATE_LDB_QUEUE				\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_DOMAIN_CMD_CREATE_LDB_QUEUE,		\
		      struct dlb_create_ldb_queue_args)
#define DLB_IOC_CREATE_DIR_QUEUE				\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_DOMAIN_CMD_CREATE_DIR_QUEUE,		\
		      struct dlb_create_dir_queue_args)
#define DLB_IOC_CREATE_LDB_PORT					\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_DOMAIN_CMD_CREATE_LDB_PORT,		\
		      struct dlb_create_ldb_port_args)
#define DLB_IOC_CREATE_DIR_PORT					\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_DOMAIN_CMD_CREATE_DIR_PORT,		\
		      struct dlb_create_dir_port_args)
#define DLB_IOC_START_DOMAIN					\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_DOMAIN_CMD_START_DOMAIN,		\
		      struct dlb_start_domain_args)
#define DLB_IOC_MAP_QID						\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_DOMAIN_CMD_MAP_QID,			\
		      struct dlb_map_qid_args)
#define DLB_IOC_UNMAP_QID					\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_DOMAIN_CMD_UNMAP_QID,			\
		      struct dlb_unmap_qid_args)
#define DLB_IOC_ENABLE_LDB_PORT					\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_DOMAIN_CMD_ENABLE_LDB_PORT,		\
		      struct dlb_enable_ldb_port_args)
#define DLB_IOC_ENABLE_DIR_PORT					\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_DOMAIN_CMD_ENABLE_DIR_PORT,		\
		      struct dlb_enable_dir_port_args)
#define DLB_IOC_DISABLE_LDB_PORT				\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_DOMAIN_CMD_DISABLE_LDB_PORT,		\
		      struct dlb_disable_ldb_port_args)
#define DLB_IOC_DISABLE_DIR_PORT				\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_DOMAIN_CMD_DISABLE_DIR_PORT,		\
		      struct dlb_disable_dir_port_args)
#define DLB_IOC_BLOCK_ON_CQ_INTERRUPT				\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_DOMAIN_CMD_BLOCK_ON_CQ_INTERRUPT,	\
		      struct dlb_block_on_cq_interrupt_args)
#define DLB_IOC_ENQUEUE_DOMAIN_ALERT				\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_DOMAIN_CMD_ENQUEUE_DOMAIN_ALERT,	\
		      struct dlb_enqueue_domain_alert_args)
#define DLB_IOC_GET_LDB_QUEUE_DEPTH				\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_DOMAIN_CMD_GET_LDB_QUEUE_DEPTH,	\
		      struct dlb_get_ldb_queue_depth_args)
#define DLB_IOC_GET_DIR_QUEUE_DEPTH				\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_DOMAIN_CMD_GET_DIR_QUEUE_DEPTH,	\
		      struct dlb_get_dir_queue_depth_args)
#define DLB_IOC_PENDING_PORT_UNMAPS				\
		_IOWR(DLB_IOC_MAGIC,				\
		      DLB_DOMAIN_CMD_PENDING_PORT_UNMAPS,	\
		      struct dlb_pending_port_unmaps_args)

#endif /* __DLB_USER_H */
