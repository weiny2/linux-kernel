/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
 * Copyright(c) 2016-2020 Intel Corporation
 */

#ifndef __DLB_USER_H
#define __DLB_USER_H

#include <linux/types.h>

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
 * DLB_CMD_CREATE_SCHED_DOMAIN: Create an DLB scheduling domain and reserve the
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

enum dlb_user_interface_commands {
	DLB_CMD_GET_DEVICE_VERSION,
	DLB_CMD_CREATE_SCHED_DOMAIN,
	DLB_CMD_GET_NUM_RESOURCES,
	DLB_CMD_GET_DRIVER_VERSION,

	/* NUM_DLB_CMD must be last */
	NUM_DLB_CMD,
};

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

#endif /* __DLB_USER_H */
