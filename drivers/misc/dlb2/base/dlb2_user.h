/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
 * Copyright(c) 2016-2020 Intel Corporation
 */

#ifndef __DLB2_USER_H
#define __DLB2_USER_H

#define DLB2_MAX_NAME_LEN 64

#include "dlb2_osdep_types.h"

enum dlb2_error {
	DLB2_ST_SUCCESS = 0,
	DLB2_ST_NAME_EXISTS,
	DLB2_ST_DOMAIN_UNAVAILABLE,
	DLB2_ST_LDB_PORTS_UNAVAILABLE,
	DLB2_ST_DIR_PORTS_UNAVAILABLE,
	DLB2_ST_LDB_QUEUES_UNAVAILABLE,
	DLB2_ST_LDB_CREDITS_UNAVAILABLE,
	DLB2_ST_DIR_CREDITS_UNAVAILABLE,
	DLB2_ST_SEQUENCE_NUMBERS_UNAVAILABLE,
	DLB2_ST_INVALID_DOMAIN_ID,
	DLB2_ST_INVALID_QID_INFLIGHT_ALLOCATION,
	DLB2_ST_ATOMIC_INFLIGHTS_UNAVAILABLE,
	DLB2_ST_HIST_LIST_ENTRIES_UNAVAILABLE,
	DLB2_ST_INVALID_LDB_QUEUE_ID,
	DLB2_ST_INVALID_CQ_DEPTH,
	DLB2_ST_INVALID_CQ_VIRT_ADDR,
	DLB2_ST_INVALID_PORT_ID,
	DLB2_ST_INVALID_QID,
	DLB2_ST_INVALID_PRIORITY,
	DLB2_ST_NO_QID_SLOTS_AVAILABLE,
	DLB2_ST_INVALID_DIR_QUEUE_ID,
	DLB2_ST_DIR_QUEUES_UNAVAILABLE,
	DLB2_ST_DOMAIN_NOT_CONFIGURED,
	DLB2_ST_INTERNAL_ERROR,
	DLB2_ST_DOMAIN_IN_USE,
	DLB2_ST_DOMAIN_NOT_FOUND,
	DLB2_ST_QUEUE_NOT_FOUND,
	DLB2_ST_DOMAIN_STARTED,
	DLB2_ST_DOMAIN_NOT_STARTED,
	DLB2_ST_LDB_PORT_REQUIRED_FOR_LDB_QUEUES,
	DLB2_ST_DOMAIN_RESET_FAILED,
	DLB2_ST_MBOX_ERROR,
	DLB2_ST_INVALID_HIST_LIST_DEPTH,
	DLB2_ST_NO_MEMORY,
	DLB2_ST_INVALID_LOCK_ID_COMP_LEVEL,
	DLB2_ST_INVALID_COS_ID,
	DLB2_ST_INVALID_SMON_ID,
	DLB2_ST_INVALID_SMON_MODE,
	DLB2_ST_INVALID_SMON_COMP_MODE,
	DLB2_ST_INVALID_SMON_CAP_MODE,
};

static const char dlb2_error_strings[][128] = {
	"DLB2_ST_SUCCESS",
	"DLB2_ST_NAME_EXISTS",
	"DLB2_ST_DOMAIN_UNAVAILABLE",
	"DLB2_ST_LDB_PORTS_UNAVAILABLE",
	"DLB2_ST_DIR_PORTS_UNAVAILABLE",
	"DLB2_ST_LDB_QUEUES_UNAVAILABLE",
	"DLB2_ST_LDB_CREDITS_UNAVAILABLE",
	"DLB2_ST_DIR_CREDITS_UNAVAILABLE",
	"DLB2_ST_SEQUENCE_NUMBERS_UNAVAILABLE",
	"DLB2_ST_INVALID_DOMAIN_ID",
	"DLB2_ST_INVALID_QID_INFLIGHT_ALLOCATION",
	"DLB2_ST_ATOMIC_INFLIGHTS_UNAVAILABLE",
	"DLB2_ST_HIST_LIST_ENTRIES_UNAVAILABLE",
	"DLB2_ST_INVALID_LDB_QUEUE_ID",
	"DLB2_ST_INVALID_CQ_DEPTH",
	"DLB2_ST_INVALID_CQ_VIRT_ADDR",
	"DLB2_ST_INVALID_PORT_ID",
	"DLB2_ST_INVALID_QID",
	"DLB2_ST_INVALID_PRIORITY",
	"DLB2_ST_NO_QID_SLOTS_AVAILABLE",
	"DLB2_ST_INVALID_DIR_QUEUE_ID",
	"DLB2_ST_DIR_QUEUES_UNAVAILABLE",
	"DLB2_ST_DOMAIN_NOT_CONFIGURED",
	"DLB2_ST_INTERNAL_ERROR",
	"DLB2_ST_DOMAIN_IN_USE",
	"DLB2_ST_DOMAIN_NOT_FOUND",
	"DLB2_ST_QUEUE_NOT_FOUND",
	"DLB2_ST_DOMAIN_STARTED",
	"DLB2_ST_DOMAIN_NOT_STARTED",
	"DLB2_ST_LDB_PORT_REQUIRED_FOR_LDB_QUEUES",
	"DLB2_ST_DOMAIN_RESET_FAILED",
	"DLB2_ST_MBOX_ERROR",
	"DLB2_ST_INVALID_HIST_LIST_DEPTH",
	"DLB2_ST_NO_MEMORY",
	"DLB2_ST_INVALID_LOCK_ID_COMP_LEVEL",
	"DLB2_ST_INVALID_COS_ID",
	"DLB2_ST_INVALID_SMON_ID",
	"DLB2_ST_INVALID_SMON_MODE",
	"DLB2_ST_INVALID_SMON_COMP_MODE",
	"DLB2_ST_INVALID_SMON_CAP_MODE",
};

struct dlb2_cmd_response {
	__u32 status; /* Interpret using enum dlb2_error */
	__u32 id;
};

/********************************/
/* 'dlb2' device file commands */
/********************************/

#define DLB2_DEVICE_VERSION(x) (((x) >> 8) & 0xFF)
#define DLB2_DEVICE_REVISION(x) ((x) & 0xFF)

enum dlb2_revisions {
	DLB2_REV_A0 = 0,
};

/*
 * DLB2_CMD_GET_DEVICE_VERSION: Query the DLB device version.
 *
 *	This ioctl interface is the same in all driver versions and is always
 *	the first ioctl.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id[7:0]: Device revision.
 *	response.id[15:8]: Device version.
 */

struct dlb2_get_device_version_args {
	/* Output parameters */
	__u64 response;
};

#define DLB2_VERSION_MAJOR_NUMBER 12
#define DLB2_VERSION_MINOR_NUMBER 0
#define DLB2_VERSION_REVISION_NUMBER 1
#define DLB2_VERSION (DLB2_VERSION_MAJOR_NUMBER << 24 | \
		      DLB2_VERSION_MINOR_NUMBER << 16 | \
		      DLB2_VERSION_REVISION_NUMBER)

#define DLB2_VERSION_GET_MAJOR_NUMBER(x) (((x) >> 24) & 0xFF)
#define DLB2_VERSION_GET_MINOR_NUMBER(x) (((x) >> 16) & 0xFF)
#define DLB2_VERSION_GET_REVISION_NUMBER(x) ((x) & 0xFFFF)

static inline __u8 dlb2_version_incompatible(__u32 version)
{
	__u8 inc;

	inc = DLB2_VERSION_GET_MAJOR_NUMBER(version) !=
		DLB2_VERSION_MAJOR_NUMBER;
	inc |= (int)DLB2_VERSION_GET_MINOR_NUMBER(version) <
		DLB2_VERSION_MINOR_NUMBER;

	return inc;
}

/*
 * DLB2_CMD_GET_DRIVER_VERSION: Query the DLB2 driver version. The major
 *	number is changed when there is an ABI-breaking change, the minor
 *	number is changed if the API is changed in a backwards-compatible way,
 *	and the revision number is changed for fixes that don't affect the API.
 *
 *	If the kernel driver's API version major number and the header's
 *	DLB2_VERSION_MAJOR_NUMBER differ, the two are incompatible, or if the
 *	major numbers match but the kernel driver's minor number is less than
 *	the header file's, they are incompatible. The DLB2_VERSION_INCOMPATIBLE
 *	macro should be used to check for compatibility.
 *
 *	This ioctl interface is the same in all driver versions. Applications
 *	should check the driver version before performing any other ioctl
 *	operations.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Driver API version. Use the DLB2_VERSION_GET_MAJOR_NUMBER,
 *		DLB2_VERSION_GET_MINOR_NUMBER, and
 *		DLB2_VERSION_GET_REVISION_NUMBER macros to interpret the field.
 */

struct dlb2_get_driver_version_args {
	/* Output parameters */
	__u64 response;
};

/*
 * DLB2_CMD_CREATE_SCHED_DOMAIN: Create a DLB 2.0 scheduling domain and reserve
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
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: domain ID.
 */
struct dlb2_create_sched_domain_args {
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
 * DLB2_CMD_GET_NUM_RESOURCES: Return the number of available resources
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
struct dlb2_get_num_resources_args {
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

enum dlb2_smons {
	DLB2_SMON_ATM,
	DLB2_SMON_CHP,
	DLB2_SMON_PM,
	DLB2_SMON_DQED,
	DLB2_SMON_LSP,
	DLB2_SMON_ROP,
	DLB2_SMON_DIR,
	DLB2_SMON_AQED,
	DLB2_SMON_QED,
	DLB2_SMON_NALB,
	DLB2_SMON_SYS0,
	DLB2_SMON_SYS1,
	DLB2_SMON_IOSF0,
	DLB2_SMON_IOSF1,

	/* NUM_DLB2_SMONS must be last */
	NUM_DLB2_SMONS,
};

/*
 * DLB2_CMD_WRITE_SMON: Write SMON counters.
 *
 *	This ioctl is not supported for virtual devices.
 *
 * Input parameters:
 * - smon_id: SMON ID (see enum dlb2_smons)
 * - sel0: Measurement target 0
 * - sel1: Measurement target 1
 * - cmpm0: Compare mode 0
 * - cmpm1: Compare mode 1
 * - cmpv0: Compare value 0
 * - cmpv1: Compare value 1
 * - mask0: Compare mask 0
 * - mask1: Compare mask 1
 * - capm0: Capture mode 0
 * - capm1: Capture mode 1
 * - cnt0: Counter 0 start value
 * - cnt1: Counter 1 start value
 * - mode: Measurement mode
 * - max_timer: Max timer value
 * - timer: Timer start value
 * - haltt: Halt on timer
 * - haltc: Halt on counter
 * - en: Enable
 * - lsp_valid_sel: Valid select (LSP only)
 * - lsp_value_sel: Value select (LSP only)
 * - lsp_compare_sel: Compare select (LSP only)
 * - padding: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb2_write_smon_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 smon_id;
	__u32 sel0;
	__u32 sel1;
	__u32 cmpm0;
	__u32 cmpm1;
	__u32 cmpv0;
	__u32 cmpv1;
	__u32 mask0;
	__u32 mask1;
	__u32 capm0;
	__u32 capm1;
	__u32 cnt0;
	__u32 cnt1;
	__u32 mode;
	__u32 max_timer;
	__u32 timer;
	__u16 haltt;
	__u16 haltc;
	__u16 en;
	__u16 lsp_valid_sel;
	__u16 lsp_value_sel;
	__u16 lsp_compare_sel;
	__u32 padding;
};

/*
 * DLB2_CMD_READ_SMON: Read SMON counters.
 *
 *	This ioctl is not supported for virtual devices.
 *
 * Input parameters:
 * - smon_id: SMON ID (see enum dlb2_smons)
 * - padding1: Reserved for future use.
 *
 * Output parameters:
 * - sel0: Measurement target 0
 * - sel1: Measurement target 1
 * - cmpm0: Compare mode 0
 * - cmpm1: Compare mode 1
 * - cmpv0: Compare value 0
 * - cmpv1: Compare value 1
 * - mask0: Compare mask 0
 * - mask1: Compare mask 1
 * - capm0: Capture mode 0
 * - capm1: Capture mode 1
 * - cnt0: Counter 0 value
 * - cnt1: Counter 1 value
 * - mode: Measurement mode
 * - max_timer: Max timer value
 * - timer: Timer start value
 * - haltt: Halt on timer
 * - haltc: Halt on counter
 * - en: Enable
 * - lsp_valid_sel: Valid select (LSP only)
 * - lsp_value_sel: Value select (LSP only)
 * - lsp_compare_sel: Compare select (LSP only)
 * - padding0: Reserved for future use.
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb2_read_smon_args {
	/* Output parameters */
	__u64 response;
	__u32 sel0;
	__u32 sel1;
	__u32 cmpm0;
	__u32 cmpm1;
	__u32 cmpv0;
	__u32 cmpv1;
	__u32 mask0;
	__u32 mask1;
	__u32 capm0;
	__u32 capm1;
	__u32 cnt0;
	__u32 cnt1;
	__u32 mode;
	__u32 max_timer;
	__u32 timer;
	__u16 haltt;
	__u16 haltc;
	__u16 en;
	__u16 lsp_valid_sel;
	__u16 lsp_value_sel;
	__u16 lsp_compare_sel;
	__u32 padding0;
	/* Input parameters */
	__u32 smon_id;
	__u32 padding1;
};

/*
 * DLB2_CMD_SET_SN_ALLOCATION: Configure a sequence number group (PF only)
 *
 * Input parameters:
 * - group: Sequence number group ID.
 * - num: Number of sequence numbers per queue.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb2_set_sn_allocation_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 group;
	__u32 num;
};

/*
 * DLB2_CMD_GET_SN_ALLOCATION: Get a sequence number group's configuration
 *
 * Input parameters:
 * - group: Sequence number group ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Specified group's number of sequence numbers per queue.
 */
struct dlb2_get_sn_allocation_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 group;
	__u32 padding0;
};

/*
 * DLB2_CMD_SET_COS_BW: Set a bandwidth allocation percentage for a
 *	load-balanced port class-of-service (PF only).
 *
 * Input parameters:
 * - cos_id: class-of-service ID, between 0 and 3 (inclusive).
 * - bandwidth: class-of-service bandwidth percentage. Total bandwidth
 *		percentages across all 4 classes cannot exceed 100%.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb2_set_cos_bw_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 cos_id;
	__u32 bandwidth;
};

/*
 * DLB2_CMD_GET_COS_BW: Get the bandwidth allocation percentage for a
 *	load-balanced port class-of-service.
 *
 * Input parameters:
 * - cos_id: class-of-service ID, between 0 and 3 (inclusive).
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Specified class's bandwidth percentage.
 */
struct dlb2_get_cos_bw_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 cos_id;
	__u32 padding0;
};

/*
 * DLB2_CMD_GET_SN_OCCUPANCY: Get a sequence number group's occupancy
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
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Specified group's number of used slots.
 */
struct dlb2_get_sn_occupancy_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 group;
	__u32 padding0;
};

enum dlb2_cq_poll_modes {
	DLB2_CQ_POLL_MODE_STD,
	DLB2_CQ_POLL_MODE_SPARSE,

	/* NUM_DLB2_CQ_POLL_MODE must be last */
	NUM_DLB2_CQ_POLL_MODE,
};

/*
 * DLB2_CMD_QUERY_CQ_POLL_MODE: Query the CQ poll mode setting
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: CQ poll mode (see enum dlb2_cq_poll_modes).
 */
struct dlb2_query_cq_poll_mode_args {
	/* Output parameters */
	__u64 response;
};

enum dlb2_user_interface_commands {
	DLB2_CMD_GET_DEVICE_VERSION,
	DLB2_CMD_CREATE_SCHED_DOMAIN,
	DLB2_CMD_GET_NUM_RESOURCES,
	DLB2_CMD_GET_DRIVER_VERSION,
	DLB2_CMD_WRITE_SMON,
	DLB2_CMD_READ_SMON,
	DLB2_CMD_SET_SN_ALLOCATION,
	DLB2_CMD_GET_SN_ALLOCATION,
	DLB2_CMD_SET_COS_BW,
	DLB2_CMD_GET_COS_BW,
	DLB2_CMD_GET_SN_OCCUPANCY,
	DLB2_CMD_QUERY_CQ_POLL_MODE,

	/* NUM_DLB2_CMD must be last */
	NUM_DLB2_CMD,
};

/*******************************/
/* 'domain' device file alerts */
/*******************************/

/* Scheduling domain device files can be read to receive domain-specific
 * notifications, for alerts such as hardware errors or device reset.
 *
 * Each alert is encoded in a 16B message. The first 8B contains the alert ID,
 * and the second 8B is optional and contains additional information.
 * Applications should cast read data to a struct dlb2_domain_alert, and
 * interpret the struct's alert_id according to dlb2_domain_alert_id. The read
 * length must be 16B, or the function will return -EINVAL.
 *
 * Reads are destructive, and in the case of multiple file descriptors for the
 * same domain device file, an alert will be read by only one of the file
 * descriptors.
 *
 * The driver stores alerts in a fixed-size alert ring until they are read. If
 * the alert ring fills completely, subsequent alerts will be dropped. It is
 * recommended that DLB2 applications dedicate a thread to perform blocking
 * reads on the device file.
 */
enum dlb2_domain_alert_id {
	/* Software issued an illegal enqueue for a port in this domain. An
	 * illegal enqueue could be:
	 * - Illegal (excess) completion
	 * - Illegal fragment
	 * - Insufficient credits
	 * aux_alert_data[7:0] contains the port ID, and aux_alert_data[15:8]
	 * contains a flag indicating whether the port is load-balanced (1) or
	 * directed (0).
	 */
	DLB2_DOMAIN_ALERT_PP_ILLEGAL_ENQ,
	/* Software issued excess CQ token pops for a port in this domain.
	 * aux_alert_data[7:0] contains the port ID, and aux_alert_data[15:8]
	 * contains a flag indicating whether the port is load-balanced (1) or
	 * directed (0).
	 */
	DLB2_DOMAIN_ALERT_PP_EXCESS_TOKEN_POPS,
	/* A enqueue contained either an invalid command encoding or a REL,
	 * REL_T, RLS, FWD, FWD_T, FRAG, or FRAG_T from a directed port.
	 *
	 * aux_alert_data[7:0] contains the port ID, and aux_alert_data[15:8]
	 * contains a flag indicating whether the port is load-balanced (1) or
	 * directed (0).
	 */
	DLB2_DOMAIN_ALERT_ILLEGAL_HCW,
	/* The QID must be valid and less than 128.
	 *
	 * aux_alert_data[7:0] contains the port ID, and aux_alert_data[15:8]
	 * contains a flag indicating whether the port is load-balanced (1) or
	 * directed (0).
	 */
	DLB2_DOMAIN_ALERT_ILLEGAL_QID,
	/* An enqueue went to a disabled QID.
	 *
	 * aux_alert_data[7:0] contains the port ID, and aux_alert_data[15:8]
	 * contains a flag indicating whether the port is load-balanced (1) or
	 * directed (0).
	 */
	DLB2_DOMAIN_ALERT_DISABLED_QID,
	/* The device containing this domain was reset. All applications using
	 * the device need to exit for the driver to complete the reset
	 * procedure.
	 *
	 * aux_alert_data doesn't contain any information for this alert.
	 */
	DLB2_DOMAIN_ALERT_DEVICE_RESET,
	/* User-space has enqueued an alert.
	 *
	 * aux_alert_data contains user-provided data.
	 */
	DLB2_DOMAIN_ALERT_USER,
	/* The watchdog timer fired for the specified port. This occurs if its
	 * CQ was not serviced for a large amount of time, likely indicating a
	 * hung thread.
	 * aux_alert_data[7:0] contains the port ID, and aux_alert_data[15:8]
	 * contains a flag indicating whether the port is load-balanced (1) or
	 * directed (0).
	 */
	DLB2_DOMAIN_ALERT_CQ_WATCHDOG_TIMEOUT,

	/* Number of DLB2 domain alerts */
	NUM_DLB2_DOMAIN_ALERTS
};

static const char dlb2_domain_alert_strings[][128] = {
	"DLB2_DOMAIN_ALERT_PP_ILLEGAL_ENQ",
	"DLB2_DOMAIN_ALERT_PP_EXCESS_TOKEN_POPS",
	"DLB2_DOMAIN_ALERT_ILLEGAL_HCW",
	"DLB2_DOMAIN_ALERT_ILLEGAL_QID",
	"DLB2_DOMAIN_ALERT_DISABLED_QID",
	"DLB2_DOMAIN_ALERT_DEVICE_RESET",
	"DLB2_DOMAIN_ALERT_USER",
	"DLB2_DOMAIN_ALERT_CQ_WATCHDOG_TIMEOUT",
};

struct dlb2_domain_alert {
	__u64 alert_id;
	__u64 aux_alert_data;
};

/*********************************/
/* 'domain' device file commands */
/*********************************/

/*
 * DLB2_DOMAIN_CMD_CREATE_LDB_QUEUE: Configure a load-balanced queue.
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
 * - depth_threshold: DLB sets two bits in the received QE to indicate the
 *	depth of the queue relative to the threshold before scheduling the
 *	QE to a CQ:
 *	- 2’b11: depth > threshold
 *	- 2’b10: threshold >= depth > 0.75 * threshold
 *	- 2’b01: 0.75 * threshold >= depth > 0.5 * threshold
 *	- 2’b00: depth <= 0.5 * threshold
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Queue ID.
 */
struct dlb2_create_ldb_queue_args {
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
 * DLB2_DOMAIN_CMD_CREATE_DIR_QUEUE: Configure a directed queue.
 * Input parameters:
 * - port_id: Port ID. If the corresponding directed port is already created,
 *	specify its ID here. Else this argument must be 0xFFFFFFFF to indicate
 *	that the queue is being created before the port.
 * - depth_threshold: DLB sets two bits in the received QE to indicate the
 *	depth of the queue relative to the threshold before scheduling the
 *	QE to a CQ:
 *	- 2’b11: depth > threshold
 *	- 2’b10: threshold >= depth > 0.75 * threshold
 *	- 2’b01: 0.75 * threshold >= depth > 0.5 * threshold
 *	- 2’b00: depth <= 0.5 * threshold
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Queue ID.
 */
struct dlb2_create_dir_queue_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__s32 port_id;
	__u32 depth_threshold;
};

/*
 * DLB2_DOMAIN_CMD_CREATE_LDB_PORT: Configure a load-balanced port.
 * Input parameters:
 * - cq_depth: Depth of the port's CQ. Must be a power-of-two between 8 and
 *	1024, inclusive.
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
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: port ID.
 */

struct dlb2_create_ldb_port_args {
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
 * DLB2_DOMAIN_CMD_CREATE_DIR_PORT: Configure a directed port.
 * Input parameters:
 * - cq_depth: Depth of the port's CQ. Must be a power-of-two between 8 and
 *	1024, inclusive.
 * - cq_depth_threshold: CQ depth interrupt threshold. A value of N means that
 *	the CQ interrupt won't fire until there are N or more outstanding CQ
 *	tokens.
 * - qid: Queue ID. If the corresponding directed queue is already created,
 *	specify its ID here. Else this argument must be 0xFFFFFFFF to indicate
 *	that the port is being created before the queue.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: Port ID.
 */
struct dlb2_create_dir_port_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u16 cq_depth;
	__u16 cq_depth_threshold;
	__s32 queue_id;
};

/*
 * DLB2_DOMAIN_CMD_START_DOMAIN: Mark the end of the domain configuration. This
 *	must be called before passing QEs into the device, and no configuration
 *	ioctls can be issued once the domain has started. Sending QEs into the
 *	device before calling this ioctl will result in undefined behavior.
 * Input parameters:
 * - (None)
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb2_start_domain_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
};

/*
 * DLB2_DOMAIN_CMD_MAP_QID: Map a load-balanced queue to a load-balanced port.
 * Input parameters:
 * - port_id: Load-balanced port ID.
 * - qid: Load-balanced queue ID.
 * - priority: Queue->port service priority.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb2_map_qid_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u32 qid;
	__u32 priority;
	__u32 padding0;
};

/*
 * DLB2_DOMAIN_CMD_UNMAP_QID: Unmap a load-balanced queue to a load-balanced
 *	port.
 * Input parameters:
 * - port_id: Load-balanced port ID.
 * - qid: Load-balanced queue ID.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb2_unmap_qid_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u32 qid;
};

/*
 * DLB2_DOMAIN_CMD_ENABLE_LDB_PORT: Enable scheduling to a load-balanced port.
 * Input parameters:
 * - port_id: Load-balanced port ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb2_enable_ldb_port_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u32 padding0;
};

/*
 * DLB2_DOMAIN_CMD_ENABLE_DIR_PORT: Enable scheduling to a directed port.
 * Input parameters:
 * - port_id: Directed port ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb2_enable_dir_port_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
};

/*
 * DLB2_DOMAIN_CMD_DISABLE_LDB_PORT: Disable scheduling to a load-balanced
 *	port.
 * Input parameters:
 * - port_id: Load-balanced port ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb2_disable_ldb_port_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u32 padding0;
};

/*
 * DLB2_DOMAIN_CMD_DISABLE_DIR_PORT: Disable scheduling to a directed port.
 * Input parameters:
 * - port_id: Directed port ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb2_disable_dir_port_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u32 padding0;
};

/*
 * DLB2_DOMAIN_CMD_BLOCK_ON_CQ_INTERRUPT: Block on a CQ interrupt until a QE
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
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb2_block_on_cq_interrupt_args {
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
 * DLB2_DOMAIN_CMD_ENQUEUE_DOMAIN_ALERT: Enqueue a domain alert that will be
 *	read by one reader thread.
 *
 * Input parameters:
 * - aux_alert_data: user-defined auxiliary data.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 */
struct dlb2_enqueue_domain_alert_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u64 aux_alert_data;
};

/*
 * DLB2_DOMAIN_CMD_GET_LDB_QUEUE_DEPTH: Get a load-balanced queue's depth.
 * Input parameters:
 * - queue_id: The load-balanced queue ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: queue depth.
 */
struct dlb2_get_ldb_queue_depth_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 queue_id;
	__u32 padding0;
};

/*
 * DLB2_DOMAIN_CMD_DIR_QUEUE_DEPTH: Get a directed queue's depth.
 * Input parameters:
 * - queue_id: The directed queue ID.
 * - padding0: Reserved for future use.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: queue depth.
 */
struct dlb2_get_dir_queue_depth_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 queue_id;
	__u32 padding0;
};

/*
 * DLB2_DOMAIN_CMD_PENDING_PORT_UNMAPS: Get number of queue unmap operations in
 *	progress for a load-balanced port.
 *
 *	Note: This is a snapshot; the number of unmap operations in progress
 *	is subject to change at any time.
 *
 * Input parameters:
 * - port_id: Load-balanced port ID.
 *
 * Output parameters:
 * - response: pointer to a struct dlb2_cmd_response.
 *	response.status: Detailed error code. In certain cases, such as if the
 *		response pointer is invalid, the driver won't set status.
 *	response.id: number of unmaps in progress.
 */
struct dlb2_pending_port_unmaps_args {
	/* Output parameters */
	__u64 response;
	/* Input parameters */
	__u32 port_id;
	__u32 padding0;
};

enum dlb2_domain_user_interface_commands {
	DLB2_DOMAIN_CMD_CREATE_LDB_QUEUE,
	DLB2_DOMAIN_CMD_CREATE_DIR_QUEUE,
	DLB2_DOMAIN_CMD_CREATE_LDB_PORT,
	DLB2_DOMAIN_CMD_CREATE_DIR_PORT,
	DLB2_DOMAIN_CMD_START_DOMAIN,
	DLB2_DOMAIN_CMD_MAP_QID,
	DLB2_DOMAIN_CMD_UNMAP_QID,
	DLB2_DOMAIN_CMD_ENABLE_LDB_PORT,
	DLB2_DOMAIN_CMD_ENABLE_DIR_PORT,
	DLB2_DOMAIN_CMD_DISABLE_LDB_PORT,
	DLB2_DOMAIN_CMD_DISABLE_DIR_PORT,
	DLB2_DOMAIN_CMD_BLOCK_ON_CQ_INTERRUPT,
	DLB2_DOMAIN_CMD_ENQUEUE_DOMAIN_ALERT,
	DLB2_DOMAIN_CMD_GET_LDB_QUEUE_DEPTH,
	DLB2_DOMAIN_CMD_GET_DIR_QUEUE_DEPTH,
	DLB2_DOMAIN_CMD_PENDING_PORT_UNMAPS,

	/* NUM_DLB2_DOMAIN_CMD must be last */
	NUM_DLB2_DOMAIN_CMD,
};

/*
 * Base addresses for memory mapping the consumer queue (CQ) memory space, and
 * producer port (PP) MMIO space. The CQ and PP addresses are per-port.
 */
#define DLB2_LDB_CQ_BASE 0x3000000
#define DLB2_LDB_CQ_MAX_SIZE 65536
#define DLB2_LDB_CQ_OFFS(id) (DLB2_LDB_CQ_BASE + (id) * DLB2_LDB_CQ_MAX_SIZE)

#define DLB2_DIR_CQ_BASE 0x3800000
#define DLB2_DIR_CQ_MAX_SIZE 65536
#define DLB2_DIR_CQ_OFFS(id) (DLB2_DIR_CQ_BASE + (id) * DLB2_DIR_CQ_MAX_SIZE)

#define DLB2_LDB_PP_BASE 0x2100000
#define DLB2_LDB_PP_MAX_SIZE 4096
#define DLB2_LDB_PP_OFFS(id) (DLB2_LDB_PP_BASE + (id) * DLB2_LDB_PP_MAX_SIZE)

#define DLB2_DIR_PP_BASE 0x2000000
#define DLB2_DIR_PP_MAX_SIZE 4096
#define DLB2_DIR_PP_OFFS(id) (DLB2_DIR_PP_BASE + (id) * DLB2_DIR_PP_MAX_SIZE)

#endif /* __DLB2_USER_H */
