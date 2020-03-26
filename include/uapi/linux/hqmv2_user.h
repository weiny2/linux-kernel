/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
 * Copyright(c) 2016-2018 Intel Corporation
 */

#ifndef __HQMV2_USER_H
#define __HQMV2_USER_H

#define HQMV2_MAX_NAME_LEN 64

#include <linux/types.h>

enum hqmv2_error {
	HQMV2_ST_SUCCESS = 0,
	HQMV2_ST_NAME_EXISTS,
	HQMV2_ST_DOMAIN_UNAVAILABLE,
	HQMV2_ST_LDB_PORTS_UNAVAILABLE,
	HQMV2_ST_DIR_PORTS_UNAVAILABLE,
	HQMV2_ST_LDB_QUEUES_UNAVAILABLE,
	HQMV2_ST_LDB_CREDITS_UNAVAILABLE,
	HQMV2_ST_DIR_CREDITS_UNAVAILABLE,
	HQMV2_ST_SEQUENCE_NUMBERS_UNAVAILABLE,
	HQMV2_ST_INVALID_DOMAIN_ID,
	HQMV2_ST_INVALID_QID_INFLIGHT_ALLOCATION,
	HQMV2_ST_ATOMIC_INFLIGHTS_UNAVAILABLE,
	HQMV2_ST_HIST_LIST_ENTRIES_UNAVAILABLE,
	HQMV2_ST_INVALID_LDB_QUEUE_ID,
	HQMV2_ST_INVALID_CQ_DEPTH,
	HQMV2_ST_INVALID_CQ_VIRT_ADDR,
	HQMV2_ST_INVALID_POP_COUNT_VIRT_ADDR,
	HQMV2_ST_INVALID_PORT_ID,
	HQMV2_ST_INVALID_QID,
	HQMV2_ST_INVALID_PRIORITY,
	HQMV2_ST_NO_QID_SLOTS_AVAILABLE,
	HQMV2_ST_INVALID_DIR_QUEUE_ID,
	HQMV2_ST_DIR_QUEUES_UNAVAILABLE,
	HQMV2_ST_DOMAIN_NOT_CONFIGURED,
	HQMV2_ST_INTERNAL_ERROR,
	HQMV2_ST_DOMAIN_IN_USE,
	HQMV2_ST_DOMAIN_NOT_FOUND,
	HQMV2_ST_QUEUE_NOT_FOUND,
	HQMV2_ST_DOMAIN_STARTED,
	HQMV2_ST_DOMAIN_NOT_STARTED,
	HQMV2_ST_INVALID_MEASUREMENT_DURATION,
	HQMV2_ST_INVALID_PERF_METRIC_GROUP_ID,
	HQMV2_ST_LDB_PORT_REQUIRED_FOR_LDB_QUEUES,
	HQMV2_ST_DOMAIN_RESET_FAILED,
	HQMV2_ST_MBOX_ERROR,
	HQMV2_ST_INVALID_HIST_LIST_DEPTH,
	HQMV2_ST_NO_MEMORY,
	HQMV2_ST_INVALID_LOCK_ID_COMP_LEVEL,
	HQMV2_ST_INVALID_COS_ID,
};

static const char hqmv2_error_strings[][128] = {
	"HQMV2_ST_SUCCESS",
	"HQMV2_ST_NAME_EXISTS",
	"HQMV2_ST_DOMAIN_UNAVAILABLE",
	"HQMV2_ST_LDB_PORTS_UNAVAILABLE",
	"HQMV2_ST_DIR_PORTS_UNAVAILABLE",
	"HQMV2_ST_LDB_QUEUES_UNAVAILABLE",
	"HQMV2_ST_LDB_CREDITS_UNAVAILABLE",
	"HQMV2_ST_DIR_CREDITS_UNAVAILABLE",
	"HQMV2_ST_SEQUENCE_NUMBERS_UNAVAILABLE",
	"HQMV2_ST_INVALID_DOMAIN_ID",
	"HQMV2_ST_INVALID_QID_INFLIGHT_ALLOCATION",
	"HQMV2_ST_ATOMIC_INFLIGHTS_UNAVAILABLE",
	"HQMV2_ST_HIST_LIST_ENTRIES_UNAVAILABLE",
	"HQMV2_ST_INVALID_LDB_QUEUE_ID",
	"HQMV2_ST_INVALID_CQ_DEPTH",
	"HQMV2_ST_INVALID_CQ_VIRT_ADDR",
	"HQMV2_ST_INVALID_POP_COUNT_VIRT_ADDR",
	"HQMV2_ST_INVALID_PORT_ID",
	"HQMV2_ST_INVALID_QID",
	"HQMV2_ST_INVALID_PRIORITY",
	"HQMV2_ST_NO_QID_SLOTS_AVAILABLE",
	"HQMV2_ST_INVALID_DIR_QUEUE_ID",
	"HQMV2_ST_DIR_QUEUES_UNAVAILABLE",
	"HQMV2_ST_DOMAIN_NOT_CONFIGURED",
	"HQMV2_ST_INTERNAL_ERROR",
	"HQMV2_ST_DOMAIN_IN_USE",
	"HQMV2_ST_DOMAIN_NOT_FOUND",
	"HQMV2_ST_QUEUE_NOT_FOUND",
	"HQMV2_ST_DOMAIN_STARTED",
	"HQMV2_ST_DOMAIN_NOT_STARTED",
	"HQMV2_ST_INVALID_MEASUREMENT_DURATION",
	"HQMV2_ST_INVALID_PERF_METRIC_GROUP_ID",
	"HQMV2_ST_LDB_PORT_REQUIRED_FOR_LDB_QUEUES",
	"HQMV2_ST_DOMAIN_RESET_FAILED",
	"HQMV2_ST_MBOX_ERROR",
	"HQMV2_ST_INVALID_HIST_LIST_DEPTH",
	"HQMV2_ST_NO_MEMORY",
	"HQMV2_ST_INVALID_LOCK_ID_COMP_LEVEL",
	"HQMV2_ST_INVALID_COS_ID",
};

struct hqmv2_cmd_response {
	__u32 status; /* Interpret using enum hqmv2_error */
	__u32 id;
};

/********************************/
/* 'hqmv2' device file commands */
/********************************/

#define HQMV2_DEVICE_VERSION(x) (((x) >> 8) & 0xFF)
#define HQMV2_DEVICE_REVISION(x) ((x) & 0xFF)

enum hqmv2_revisions {
	HQMV2_REV_A0 = 0,
};

/*
 * HQMV2_CMD_GET_DEVICE_VERSION: Query the HQM device version.
 *
 *	This ioctl interface is the same in all driver versions and is always
 *	the first ioctl.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id[7:0]: Device revision.
 *	response.id[15:8]: Device version.
 */

struct hqmv2_get_device_version_args {
	/* Output parameters */
	__u64 response;
};

#define HQMV2_VERSION_MAJOR_NUMBER 9
#define HQMV2_VERSION_MINOR_NUMBER 0
#define HQMV2_VERSION_REVISION_NUMBER 3
#define HQMV2_VERSION (HQMV2_VERSION_MAJOR_NUMBER << 24 | \
		     HQMV2_VERSION_MINOR_NUMBER << 16 | \
		     HQMV2_VERSION_REVISION_NUMBER)

#define HQMV2_VERSION_GET_MAJOR_NUMBER(x) (((x) >> 24) & 0xFF)
#define HQMV2_VERSION_GET_MINOR_NUMBER(x) (((x) >> 16) & 0xFF)
#define HQMV2_VERSION_GET_REVISION_NUMBER(x) ((x) & 0xFFFF)

static inline __u8 hqmv2_version_incompatible(__u32 version)
{
	__u8 inc;

	inc = HQMV2_VERSION_GET_MAJOR_NUMBER(version) !=
		HQMV2_VERSION_MAJOR_NUMBER;
	inc |= (int)HQMV2_VERSION_GET_MINOR_NUMBER(version) <
		HQMV2_VERSION_MINOR_NUMBER;

	return inc;
}

/*
 * HQMV2_CMD_GET_DRIVER_VERSION: Query the HQMV2 driver version. The major
 *	number is changed when there is an ABI-breaking change, the minor
 *	number is changed if the API is changed in a backwards-compatible way,
 *	and the revision number is changed for fixes that don't affect the API.
 *
 *	If the kernel driver's API version major number and the header's
 *	HQMV2_VERSION_MAJOR_NUMBER differ, the two are incompatible, or if the
 *	major numbers match but the kernel driver's minor number is less than
 *	the header file's, they are incompatible. The HQMV2_VERSION_INCOMPATIBLE
 *	macro should be used to check for compatibility.
 *
 *	This ioctl interface is the same in all driver versions. Applications
 *	should check the driver version before performing any other ioctl
 *	operations.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Driver API version. Use the HQMV2_VERSION_GET_MAJOR_NUMBER,
 *		HQMV2_VERSION_GET_MINOR_NUMBER, and
 *		HQMV2_VERSION_GET_REVISION_NUMBER macros to interpret the field.
 */

struct hqmv2_get_driver_version_args {
	/* Output parameters */
	__u64 response;
};

/*
 * HQMV2_CMD_CREATE_SCHED_DOMAIN: Create an HQMV2 scheduling domain and reserve
 *	its hardware resources.
 *
 * Input parameters:
 * - num_ldb_queues: Number of load-balanced queues.
 * - num_ldb_ports: Number of load-balanced ports that can be allocated from
 *	from any class-of-service with available ports.
 * - num_cos0_ldb_ports: Number of load-balanced ports from class-of-service 0.
 * - num_cos1_ldb_ports: Number of load-balanced ports from class-of-service 1.
 * - num_cos2_ldb_ports: Number of load-balanced ports from class-of-service 2.
 * - num_cos3_ldb_ports: Number of load-balanced ports from class-of-service 3.
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
 * - cos_strict: If set, return an error if there are insufficient ports in
 *	class-of-service N to satisfy the num_ldb_ports_cosN argument. If
 *	unset, attempt to fulfill num_ldb_ports_cosN arguments from other
 *	classes-of-service if class N does not contain enough free ports.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: domain ID.
 */
struct hqmv2_create_sched_domain_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 num_ldb_queues;
	__u32 num_ldb_ports;
	__u32 num_cos0_ldb_ports;
	__u32 num_cos1_ldb_ports;
	__u32 num_cos2_ldb_ports;
	__u32 num_cos3_ldb_ports;
	__u32 num_dir_ports;
	__u32 num_atomic_inflights;
	__u32 num_hist_list_entries;
	__u32 num_ldb_credits;
	__u32 num_dir_credits;
	__u8 cos_strict;
	__u8 padding0[3];
};

/*
 * HQMV2_CMD_GET_NUM_RESOURCES: Return the number of available resources
 *	(queues, ports, etc.) that this device owns.
 *
 * Output parameters:
 * - num_domains: Number of available scheduling domains.
 * - num_ldb_queues: Number of available load-balanced queues.
 * - num_ldb_ports: Total number of available load-balanced ports.
 * - num_cos0_ldb_ports: Number of available load-balanced ports from
 *	class-of-service 0.
 * - num_cos1_ldb_ports: Number of available load-balanced ports from
 *	class-of-service 1.
 * - num_cos2_ldb_ports: Number of available load-balanced ports from
 *	class-of-service 2.
 * - num_cos3_ldb_ports: Number of available load-balanced ports from
 *	class-of-service 3.
 * - num_dir_ports: Number of available directed ports. There is one directed
 *	queue for every directed port.
 * - num_atomic_inflights: Amount of available temporary atomic QE storage.
 * - num_hist_list_entries: Amount of history list storage.
 * - max_contiguous_hist_list_entries: History list storage is allocated in
 *	a contiguous chunk, and this return value is the longest available
 *	contiguous range of history list entries.
 * - num_ldb_credits: Amount of available load-balanced QE storage.
 * - num_dir_credits: Amount of available directed QE storage.
 */
struct hqmv2_get_num_resources_args {
	/* Output parameters */
	__u32 num_sched_domains;
	__u32 num_ldb_queues;
	__u32 num_ldb_ports;
	__u32 num_cos0_ldb_ports;
	__u32 num_cos1_ldb_ports;
	__u32 num_cos2_ldb_ports;
	__u32 num_cos3_ldb_ports;
	__u32 num_dir_ports;
	__u32 num_atomic_inflights;
	__u32 num_hist_list_entries;
	__u32 max_contiguous_hist_list_entries;
	__u32 num_ldb_credits;
	__u32 num_dir_credits;
};

/*
 * HQMV2_CMD_SAMPLE_PERF_COUNTERS: Gather a set of HQMV2 performance data by
 *	enabling performance counters for a user-specified measurement duration.
 *	This ioctl is blocking; the calling thread sleeps in the kernel driver
 *	for the duration of the measurement, then writes the data to user
 *	memory before returning.
 *
 *	Certain metrics cannot be measured simultaneously, so multiple
 *	invocations of this command are necessary to gather all metrics.
 *	Metrics that can be collected simultaneously are grouped together in
 *	struct hqmv2_perf_metric_group_X.
 *
 *	The driver allows only one active measurement at a time. If a thread
 *	calls this command while a measurement is ongoing, the thread will
 *	block until the original measurement completes.
 *
 * Input parameters:
 * - measurement_duration_us: Duration, in microseconds, of the
 *	measurement period. The duration must be between 1us and 60s,
 *	inclusive.
 * - perf_metric_group_id: ID of the metric group to measure.
 * - perf_metric_group_data: Pointer to union hqmv2_perf_metric_group_data
 *	structure. The driver will interpret the union according to
 *	perf_metric_group_ID.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct hqmv2_perf_metric_group_0 {
	__u32 hqmv2_iosf_to_sys_enq_count;
	__u32 hqmv2_sys_to_iosf_deq_count;
	__u32 hqmv2_sys_to_hqmv2_enq_count;
	__u32 hqmv2_hqmv2_to_sys_deq_count;
	__u64 hqmv2_ldb_sched_count;
	__u64 hqmv2_dir_sched_count;
	__u64 hqmv2_ldb_cq_sched_count[64];
	__u64 hqmv2_dir_cq_sched_count[128];
};

struct hqmv2_perf_metric_group_1 {
	__u32 hqmv2_push_ptr_update_count;
};

struct hqmv2_perf_metric_group_2 {
	__u32 hqmv2_avg_hist_list_depth;
};

struct hqmv2_perf_metric_group_3 {
	__u32 hqmv2_avg_qed_depth;
};

struct hqmv2_perf_metric_group_4 {
	__u32 hqmv2_avg_dqed_depth;
};

struct hqmv2_perf_metric_group_5 {
	__u32 hqmv2_noop_hcw_count;
	__u32 hqmv2_bat_t_hcw_count;
};

struct hqmv2_perf_metric_group_6 {
	__u32 hqmv2_comp_hcw_count;
	__u32 hqmv2_comp_t_hcw_count;
};

struct hqmv2_perf_metric_group_7 {
	__u32 hqmv2_enq_hcw_count;
	__u32 hqmv2_enq_t_hcw_count;
};

struct hqmv2_perf_metric_group_8 {
	__u32 hqmv2_renq_hcw_count;
	__u32 hqmv2_renq_t_hcw_count;
};

struct hqmv2_perf_metric_group_9 {
	__u32 hqmv2_rel_hcw_count;
};

struct hqmv2_perf_metric_group_10 {
	__u32 hqmv2_frag_hcw_count;
	__u32 hqmv2_frag_t_hcw_count;
};

union hqmv2_perf_metric_group_data {
	struct hqmv2_perf_metric_group_0 group_0;
	struct hqmv2_perf_metric_group_1 group_1;
	struct hqmv2_perf_metric_group_2 group_2;
	struct hqmv2_perf_metric_group_3 group_3;
	struct hqmv2_perf_metric_group_4 group_4;
	struct hqmv2_perf_metric_group_5 group_5;
	struct hqmv2_perf_metric_group_6 group_6;
	struct hqmv2_perf_metric_group_7 group_7;
	struct hqmv2_perf_metric_group_8 group_8;
	struct hqmv2_perf_metric_group_9 group_9;
	struct hqmv2_perf_metric_group_10 group_10;
};

struct hqmv2_sample_perf_counters_args {
	/* Output parameters */
	__u64 elapsed_time_us;
	__u64 response;
	/* Input parameters */
	__u32 measurement_duration_us;
	__u32 perf_metric_group_id;
	__u64 perf_metric_group_data;
};

/*
 * HQMV2_CMD_SET_SN_ALLOCATION: Configure a sequence number group (PF only)
 *
 * Input parameters:
 * - group: Sequence number group ID.
 * - num: Number of sequence numbers per queue.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct hqmv2_set_sn_allocation_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 group;
	__u32 num;
};

/*
 * HQMV2_CMD_GET_SN_ALLOCATION: Get a sequence number group's configuration
 *
 * Input parameters:
 * - group: Sequence number group ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Specified group's number of sequence numbers per queue.
 */
struct hqmv2_get_sn_allocation_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 group;
	__u32 padding0;
};

/*
 * HQMV2_CMD_SET_COS_BW: Set a bandwidth allocation percentage for a
 *	load-balanced port class-of-service (PF only).
 *
 * Input parameters:
 * - cos_id: class-of-service ID, between 0 and 3 (inclusive).
 * - bandwidth: class-of-service bandwidth percentage. Total bandwidth
 *		percentages across all 4 classes cannot exceed 100%.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct hqmv2_set_cos_bw_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 cos_id;
	__u32 bandwidth;
};

/*
 * HQMV2_CMD_GET_COS_BW: Get the bandwidth allocation percentage for a
 *	load-balanced port class-of-service.
 *
 * Input parameters:
 * - cos_id: class-of-service ID, between 0 and 3 (inclusive).
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Specified class's bandwidth percentage.
 */
struct hqmv2_get_cos_bw_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 cos_id;
	__u32 padding0;
};

enum hqmv2_user_interface_commands {
	HQMV2_CMD_GET_DEVICE_VERSION,
	HQMV2_CMD_CREATE_SCHED_DOMAIN,
	HQMV2_CMD_GET_NUM_RESOURCES,
	HQMV2_CMD_GET_DRIVER_VERSION,
	HQMV2_CMD_SAMPLE_PERF_COUNTERS,
	HQMV2_CMD_SET_SN_ALLOCATION,
	HQMV2_CMD_GET_SN_ALLOCATION,
	HQMV2_CMD_SET_COS_BW,
	HQMV2_CMD_GET_COS_BW,

	/* NUM_HQMV2_CMD must be last */
	NUM_HQMV2_CMD,
};

/*******************************/
/* 'domain' device file alerts */
/*******************************/

/* Scheduling domain device files can be read to receive domain-specific
 * notifications, for alerts such as hardware errors or device reset.
 *
 * Each alert is encoded in a 16B message. The first 8B contains the alert ID,
 * and the second 8B is optional and contains additional information.
 * Applications should cast read data to a struct hqmv2_domain_alert, and
 * interpret the struct's alert_id according to hqmv2_domain_alert_id. The read
 * length must be 16B, or the function will return -EINVAL.
 *
 * Reads are destructive, and in the case of multiple file descriptors for the
 * same domain device file, an alert will be read by only one of the file
 * descriptors.
 *
 * The driver stores alerts in a fixed-size alert ring until they are read. If
 * the alert ring fills completely, subsequent alerts will be dropped. It is
 * recommended that HQMV2 applications dedicate a thread to perform blocking
 * reads on the device file.
 */
enum hqmv2_domain_alert_id {
	/* Software issued an illegal enqueue for a port in this domain. An
	 * illegal enqueue could be:
	 * - Illegal (excess) completion
	 * - Illegal fragment
	 * - Insufficient credits
	 * aux_alert_data[7:0] contains the port ID, and aux_alert_data[15:8]
	 * contains a flag indicating whether the port is load-balanced (1) or
	 * directed (0).
	 */
	HQMV2_DOMAIN_ALERT_PP_ILLEGAL_ENQ,
	/* Software issued excess CQ token pops for a port in this domain.
	 * aux_alert_data[7:0] contains the port ID, and aux_alert_data[15:8]
	 * contains a flag indicating whether the port is load-balanced (1) or
	 * directed (0).
	 */
	HQMV2_DOMAIN_ALERT_PP_EXCESS_TOKEN_POPS,
	/* A enqueue contained either an invalid command encoding or a REL,
	 * REL_T, RLS, FWD, FWD_T, FRAG, or FRAG_T from a directed port.
	 *
	 * aux_alert_data[7:0] contains the port ID, and aux_alert_data[15:8]
	 * contains a flag indicating whether the port is load-balanced (1) or
	 * directed (0).
	 */
	HQMV2_DOMAIN_ALERT_ILLEGAL_HCW,
	/* The QID must be valid and less than 128.
	 *
	 * aux_alert_data[7:0] contains the port ID, and aux_alert_data[15:8]
	 * contains a flag indicating whether the port is load-balanced (1) or
	 * directed (0).
	 */
	HQMV2_DOMAIN_ALERT_ILLEGAL_QID,
	/* An enqueue went to a disabled QID.
	 *
	 * aux_alert_data[7:0] contains the port ID, and aux_alert_data[15:8]
	 * contains a flag indicating whether the port is load-balanced (1) or
	 * directed (0).
	 */
	HQMV2_DOMAIN_ALERT_DISABLED_QID,
	/* The device containing this domain was reset. All applications using
	 * the device need to exit for the driver to complete the reset
	 * procedure.
	 *
	 * aux_alert_data doesn't contain any information for this alert.
	 */
	HQMV2_DOMAIN_ALERT_DEVICE_RESET,
	/* User-space has enqueued an alert.
	 *
	 * aux_alert_data contains user-provided data.
	 */
	HQMV2_DOMAIN_ALERT_USER,

	/* Number of HQMV2 domain alerts */
	NUM_HQMV2_DOMAIN_ALERTS
};

static const char hqmv2_domain_alert_strings[][128] = {
	"HQMV2_DOMAIN_ALERT_PP_ILLEGAL_ENQ",
	"HQMV2_DOMAIN_ALERT_PP_EXCESS_TOKEN_POPS",
	"HQMV2_DOMAIN_ALERT_ILLEGAL_HCW",
	"HQMV2_DOMAIN_ALERT_ILLEGAL_QID",
	"HQMV2_DOMAIN_ALERT_DISABLED_QID",
	"HQMV2_DOMAIN_ALERT_DEVICE_RESET",
	"HQMV2_DOMAIN_ALERT_USER",
};

struct hqmv2_domain_alert {
	__u64 alert_id;
	__u64 aux_alert_data;
};

/*********************************/
/* 'domain' device file commands */
/*********************************/

/*
 * HQMV2_DOMAIN_CMD_CREATE_LDB_QUEUE: Configure a load-balanced queue.
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
 * - lock_id_comp_level: Lock ID compression level. Specifies the number of
 *	unique lock IDs the queue should compress down to. Valid compression
 *	levels: 0, 64, 128, 256, 512, 1k, 2k, 4k, 64k. If lock_id_comp_level is
 *	0, the queue won't compress its lock IDs.
 * - depth_threshold: HQM sets two bits in the received QE to indicate the
 *	depth of the queue relative to the threshold before scheduling the
 *	QE to a CQ:
 *	- 2’b11: depth > threshold
 *	- 2’b10: threshold >= depth > 0.75 * threshold
 *	- 2’b01: 0.75 * threshold >= depth > 0.5 * threshold
 *	- 2’b00: depth <= 0.5 * threshold
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Queue ID.
 */
struct hqmv2_create_ldb_queue_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 num_sequence_numbers;
	__u32 num_qid_inflights;
	__u32 num_atomic_inflights;
	__u32 lock_id_comp_level;
	__u32 depth_threshold;
	__u32 padding0;
};

/*
 * HQMV2_DOMAIN_CMD_CREATE_DIR_QUEUE: Configure a directed queue.
 * Input parameters:
 * - port_id: Port ID. If the corresponding directed port is already created,
 *	specify its ID here. Else this argument must be 0xFFFFFFFF to indicate
 *	that the queue is being created before the port.
 * - depth_threshold: HQM sets two bits in the received QE to indicate the
 *	depth of the queue relative to the threshold before scheduling the
 *	QE to a CQ:
 *	- 2’b11: depth > threshold
 *	- 2’b10: threshold >= depth > 0.75 * threshold
 *	- 2’b01: 0.75 * threshold >= depth > 0.5 * threshold
 *	- 2’b00: depth <= 0.5 * threshold
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Queue ID.
 */
struct hqmv2_create_dir_queue_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__s32 port_id;
	__u32 depth_threshold;
};

/*
 * HQMV2_DOMAIN_CMD_CREATE_LDB_PORT: Configure a load-balanced port.
 * Input parameters:
 * - cq_depth: Depth of the port's CQ. Must be a power-of-two between 8 and
 *	256, inclusive.
 * - cq_depth_threshold: CQ depth interrupt threshold. A value of N means that
 *	the CQ interrupt won't fire until there are N or more outstanding CQ
 *	tokens.
 * - num_hist_list_entries: Number of history list entries. This must be
 *	greater than or equal cq_depth.
 * - cos_id: class-of-service to allocate this port from. Must be between 0 and
 *	3, inclusive.
 * - cos_strict: If set, return an error if there are no available ports in the
 *	requested class-of-service. Else, allocate the port from a different
 *	class-of-service if the requested class has no available ports.
 *
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: port ID.
 */

struct hqmv2_create_ldb_port_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u16 cq_depth;
	__u16 cq_depth_threshold;
	__u16 cq_history_list_size;
	__u8 cos_id;
	__u8 cos_strict;
};

/*
 * HQMV2_DOMAIN_CMD_CREATE_DIR_PORT: Configure a directed port.
 * Input parameters:
 * - cq_depth: Depth of the port's CQ. Must be a power-of-two between 8 and
 *	256, inclusive.
 * - cq_depth_threshold: CQ depth interrupt threshold. A value of N means that
 *	the CQ interrupt won't fire until there are N or more outstanding CQ
 *	tokens.
 * - qid: Queue ID. If the corresponding directed queue is already created,
 *	specify its ID here. Else this argument must be 0xFFFFFFFF to indicate
 *	that the port is being created before the queue.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Port ID.
 */
struct hqmv2_create_dir_port_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u16 cq_depth;
	__u16 cq_depth_threshold;
	__s32 queue_id;
};

/*
 * HQMV2_DOMAIN_CMD_START_DOMAIN: Mark the end of the domain configuration. This
 *	must be called before passing QEs into the device, and no configuration
 *	ioctls can be issued once the domain has started. Sending QEs into the
 *	device before calling this ioctl will result in undefined behavior.
 * Input parameters:
 * - (None)
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct hqmv2_start_domain_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
};

/*
 * HQMV2_DOMAIN_CMD_MAP_QID: Map a load-balanced queue to a load-balanced port.
 * Input parameters:
 * - port_id: Load-balanced port ID.
 * - qid: Load-balanced queue ID.
 * - priority: Queue->port service priority.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct hqmv2_map_qid_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u32 qid;
	__u32 priority;
	__u32 padding0;
};

/*
 * HQMV2_DOMAIN_CMD_UNMAP_QID: Unmap a load-balanced queue to a load-balanced
 *	port.
 * Input parameters:
 * - port_id: Load-balanced port ID.
 * - qid: Load-balanced queue ID.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct hqmv2_unmap_qid_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u32 qid;
};

/*
 * HQMV2_DOMAIN_CMD_ENABLE_LDB_PORT: Enable scheduling to a load-balanced port.
 * Input parameters:
 * - port_id: Load-balanced port ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct hqmv2_enable_ldb_port_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u32 padding0;
};

/*
 * HQMV2_DOMAIN_CMD_ENABLE_DIR_PORT: Enable scheduling to a directed port.
 * Input parameters:
 * - port_id: Directed port ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct hqmv2_enable_dir_port_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
};

/*
 * HQMV2_DOMAIN_CMD_DISABLE_LDB_PORT: Disable scheduling to a load-balanced
 *	port.
 * Input parameters:
 * - port_id: Load-balanced port ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct hqmv2_disable_ldb_port_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u32 padding0;
};

/*
 * HQMV2_DOMAIN_CMD_DISABLE_DIR_PORT: Disable scheduling to a directed port.
 * Input parameters:
 * - port_id: Directed port ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct hqmv2_disable_dir_port_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u32 padding0;
};

/*
 * HQMV2_DOMAIN_CMD_BLOCK_ON_CQ_INTERRUPT: Block on a CQ interrupt until a QE
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
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct hqmv2_block_on_cq_interrupt_args {
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
 * HQMV2_DOMAIN_CMD_ENQUEUE_DOMAIN_ALERT: Enqueue a domain alert that will be
 *	read by one reader thread.
 *
 * Input parameters:
 * - aux_alert_data: user-defined auxiliary data.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct hqmv2_enqueue_domain_alert_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u64 aux_alert_data;
};

/*
 * HQMV2_DOMAIN_CMD_GET_LDB_QUEUE_DEPTH: Get a load-balanced queue's depth.
 * Input parameters:
 * - queue_id: The load-balanced queue ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: queue depth.
 */
struct hqmv2_get_ldb_queue_depth_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 queue_id;
	__u32 padding0;
};

/*
 * HQMV2_DOMAIN_CMD_DIR_QUEUE_DEPTH: Get a directed queue's depth.
 * Input parameters:
 * - queue_id: The directed queue ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: queue depth.
 */
struct hqmv2_get_dir_queue_depth_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 queue_id;
	__u32 padding0;
};

/*
 * HQMV2_DOMAIN_CMD_PENDING_PORT_UNMAPS: Get number of queue unmap operations in
 *	progress for a load-balanced port.
 *
 *	Note: This is a snapshot; the number of unmap operations in progress
 *	is subject to change at any time.
 *
 * Input parameters:
 * - port_id: Load-balanced port ID.
 *
 * Output parameters:
 * - response: pointer to a struct hqmv2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: number of unmaps in progress.
 */
struct hqmv2_pending_port_unmaps_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u32 padding0;
};

enum hqmv2_domain_user_interface_commands {
	HQMV2_DOMAIN_CMD_CREATE_LDB_QUEUE,
	HQMV2_DOMAIN_CMD_CREATE_DIR_QUEUE,
	HQMV2_DOMAIN_CMD_CREATE_LDB_PORT,
	HQMV2_DOMAIN_CMD_CREATE_DIR_PORT,
	HQMV2_DOMAIN_CMD_START_DOMAIN,
	HQMV2_DOMAIN_CMD_MAP_QID,
	HQMV2_DOMAIN_CMD_UNMAP_QID,
	HQMV2_DOMAIN_CMD_ENABLE_LDB_PORT,
	HQMV2_DOMAIN_CMD_ENABLE_DIR_PORT,
	HQMV2_DOMAIN_CMD_DISABLE_LDB_PORT,
	HQMV2_DOMAIN_CMD_DISABLE_DIR_PORT,
	HQMV2_DOMAIN_CMD_BLOCK_ON_CQ_INTERRUPT,
	HQMV2_DOMAIN_CMD_ENQUEUE_DOMAIN_ALERT,
	HQMV2_DOMAIN_CMD_GET_LDB_QUEUE_DEPTH,
	HQMV2_DOMAIN_CMD_GET_DIR_QUEUE_DEPTH,
	HQMV2_DOMAIN_CMD_PENDING_PORT_UNMAPS,

	/* NUM_HQMV2_DOMAIN_CMD must be last */
	NUM_HQMV2_DOMAIN_CMD,
};

/*
 * Base addresses for memory mapping the consumer queue (CQ) and popcount (PC)
 * memory space, and producer port (PP) MMIO space. The CQ, PC, and PP addresses
 * are per-port. Every address is page-separated (e.g. LDB PP 0 is at 0x2100000
 * and LDB PP 1 is at 0x2101000).
 */
#define HQMV2_LDB_CQ_BASE 0x2500000
#define HQMV2_DIR_CQ_BASE 0x2400000
#define HQMV2_LDB_PC_BASE 0x2300000
#define HQMV2_DIR_PC_BASE 0x2200000
#define HQMV2_LDB_PP_BASE 0x2100000
#define HQMV2_DIR_PP_BASE 0x2000000
#define HQMV2_NQ_HP_UPD_BASE 0x10000000
#define HQMV2_HBM_TP_UPD_BASE 0x11000000

/*******************/
/* hqmv2 ioctl codes */
/*******************/

#define HQMV2_IOC_MAGIC  'h'

#define HQMV2_IOC_GET_DEVICE_VERSION				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_CMD_GET_DEVICE_VERSION,		\
		      struct hqmv2_get_driver_version_args)
#define HQMV2_IOC_CREATE_SCHED_DOMAIN				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_CMD_CREATE_SCHED_DOMAIN,		\
		      struct hqmv2_create_sched_domain_args)
#define HQMV2_IOC_GET_NUM_RESOURCES				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_CMD_GET_NUM_RESOURCES,		\
		      struct hqmv2_get_num_resources_args)
#define HQMV2_IOC_GET_DRIVER_VERSION				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_CMD_GET_DRIVER_VERSION,		\
		      struct hqmv2_get_driver_version_args)
#define HQMV2_IOC_SAMPLE_PERF_COUNTERS				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_CMD_SAMPLE_PERF_COUNTERS,		\
		      struct hqmv2_sample_perf_counters_args)
#define HQMV2_IOC_SET_SN_ALLOCATION				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_CMD_SET_SN_ALLOCATION,		\
		      struct hqmv2_set_sn_allocation_args)
#define HQMV2_IOC_GET_SN_ALLOCATION				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_CMD_GET_SN_ALLOCATION,		\
		      struct hqmv2_get_sn_allocation_args)
#define HQMV2_IOC_SET_COS_BW					\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_CMD_SET_COS_BW,			\
		      struct hqmv2_set_cos_bw_args)
#define HQMV2_IOC_GET_COS_BW					\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_CMD_GET_COS_BW,			\
		      struct hqmv2_get_cos_bw_args)
#define HQMV2_IOC_CREATE_LDB_POOL				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_DOMAIN_CMD_CREATE_LDB_POOL,		\
		      struct hqmv2_create_ldb_pool_args)
#define HQMV2_IOC_CREATE_DIR_POOL				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_DOMAIN_CMD_CREATE_DIR_POOL,		\
		      struct hqmv2_create_dir_pool_args)
#define HQMV2_IOC_CREATE_LDB_QUEUE				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_DOMAIN_CMD_CREATE_LDB_QUEUE,	\
		      struct hqmv2_create_ldb_queue_args)
#define HQMV2_IOC_CREATE_DIR_QUEUE				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_DOMAIN_CMD_CREATE_DIR_QUEUE,	\
		      struct hqmv2_create_dir_queue_args)
#define HQMV2_IOC_CREATE_LDB_PORT				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_DOMAIN_CMD_CREATE_LDB_PORT,		\
		      struct hqmv2_create_ldb_port_args)
#define HQMV2_IOC_CREATE_DIR_PORT				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_DOMAIN_CMD_CREATE_DIR_PORT,		\
		      struct hqmv2_create_dir_port_args)
#define HQMV2_IOC_START_DOMAIN					\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_DOMAIN_CMD_START_DOMAIN,		\
		      struct hqmv2_start_domain_args)
#define HQMV2_IOC_MAP_QID					\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_DOMAIN_CMD_MAP_QID,			\
		      struct hqmv2_map_qid_args)
#define HQMV2_IOC_UNMAP_QID					\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_DOMAIN_CMD_UNMAP_QID,		\
		      struct hqmv2_unmap_qid_args)
#define HQMV2_IOC_ENABLE_LDB_PORT				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_DOMAIN_CMD_ENABLE_LDB_PORT,		\
		      struct hqmv2_enable_ldb_port_args)
#define HQMV2_IOC_ENABLE_DIR_PORT				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_DOMAIN_CMD_ENABLE_DIR_PORT,		\
		      struct hqmv2_enable_dir_port_args)
#define HQMV2_IOC_DISABLE_LDB_PORT				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_DOMAIN_CMD_DISABLE_LDB_PORT,	\
		      struct hqmv2_disable_ldb_port_args)
#define HQMV2_IOC_DISABLE_DIR_PORT				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_DOMAIN_CMD_DISABLE_DIR_PORT,	\
		      struct hqmv2_disable_dir_port_args)
#define HQMV2_IOC_BLOCK_ON_CQ_INTERRUPT				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_DOMAIN_CMD_BLOCK_ON_CQ_INTERRUPT,	\
		      struct hqmv2_block_on_cq_interrupt_args)
#define HQMV2_IOC_ENQUEUE_DOMAIN_ALERT				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_DOMAIN_CMD_ENQUEUE_DOMAIN_ALERT,	\
		      struct hqmv2_enqueue_domain_alert_args)
#define HQMV2_IOC_GET_LDB_QUEUE_DEPTH				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_DOMAIN_CMD_GET_LDB_QUEUE_DEPTH,	\
		      struct hqmv2_get_ldb_queue_depth_args)
#define HQMV2_IOC_GET_DIR_QUEUE_DEPTH				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_DOMAIN_CMD_GET_DIR_QUEUE_DEPTH,	\
		      struct hqmv2_get_dir_queue_depth_args)
#define HQMV2_IOC_PENDING_PORT_UNMAPS				\
		_IOWR(HQMV2_IOC_MAGIC,				\
		      HQMV2_DOMAIN_CMD_PENDING_PORT_UNMAPS,	\
		      struct hqmv2_pending_port_unmaps_args)
#endif /* __HQMV2_USER_H */
