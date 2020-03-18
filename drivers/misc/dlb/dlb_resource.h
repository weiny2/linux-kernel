/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
 * Copyright(c) 2016-2018 Intel Corporation
 */

#ifndef __DLB_RESOURCE_H
#define __DLB_RESOURCE_H

#include <linux/types.h>

#include <uapi/linux/dlb_user.h>

#include "dlb_hw_types.h"

/**
 * dlb_resource_init() - initialize the device
 * @hw: pointer to struct dlb_hw.
 *
 * This function initializes the device's software state (pointed to by the hw
 * argument) and programs global scheduling QoS registers. This function should
 * be called during driver initialization.
 *
 * The dlb_hw struct must be unique per DLB device and persist until the device
 * is reset.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 */
int dlb_resource_init(struct dlb_hw *hw);

/**
 * dlb_resource_free() - free device state memory
 * @hw: dlb_hw handle for a particular device.
 *
 * This function frees software state pointed to by dlb_hw. This function
 * should be called when resetting the device or unloading the driver.
 */
void dlb_resource_free(struct dlb_hw *hw);

/**
 * dlb_hw_create_sched_domain() - create a scheduling domain
 * @hw: dlb_hw handle for a particular device.
 * @args: scheduling domain creation arguments.
 * @resp: response structure.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function creates a scheduling domain containing the resources specified
 * in args. The individual resources (queues, ports, credit pools) can be
 * configured after creating a scheduling domain.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum dlb_error. If successful, resp->id
 * contains the domain ID.
 *
 * Note: resp->id contains a virtual ID if vf_request is true.
 *
 * Errors:
 * EINVAL - A requested resource is unavailable, or the requested domain name
 *	    is already in use.
 * EFAULT - Internal error (resp->status not set).
 */
int dlb_hw_create_sched_domain(struct dlb_hw *hw,
			       struct dlb_create_sched_domain_args *args,
			       struct dlb_cmd_response *resp,
			       bool vf_request,
			       unsigned int vf_id);

/**
 * dlb_hw_create_ldb_pool() - create a load-balanced credit pool
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: credit pool creation arguments.
 * @resp: response structure.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function creates a load-balanced credit pool containing the number of
 * requested credits.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum dlb_error. If successful, resp->id
 * contains the pool ID.
 *
 * Note: resp->id contains a virtual ID if vf_request is true.
 *
 * Errors:
 * EINVAL - A requested resource is unavailable, the domain is not configured,
 *	    or the domain has already been started.
 * EFAULT - Internal error (resp->status not set).
 */
int dlb_hw_create_ldb_pool(struct dlb_hw *hw,
			   u32 domain_id,
			   struct dlb_create_ldb_pool_args *args,
			   struct dlb_cmd_response *resp,
			   bool vf_request,
			   unsigned int vf_id);

/**
 * dlb_hw_create_dir_pool() - create a directed credit pool
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: credit pool creation arguments.
 * @resp: response structure.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function creates a directed credit pool containing the number of
 * requested credits.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum dlb_error. If successful, resp->id
 * contains the pool ID.
 *
 * Note: resp->id contains a virtual ID if vf_request is true.
 *
 * Errors:
 * EINVAL - A requested resource is unavailable, the domain is not configured,
 *	    or the domain has already been started.
 * EFAULT - Internal error (resp->status not set).
 */
int dlb_hw_create_dir_pool(struct dlb_hw *hw,
			   u32 domain_id,
			   struct dlb_create_dir_pool_args *args,
			   struct dlb_cmd_response *resp,
			   bool vf_request,
			   unsigned int vf_id);

/**
 * dlb_hw_create_ldb_queue() - create a load-balanced queue
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: queue creation arguments.
 * @resp: response structure.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function creates a load-balanced queue.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum dlb_error. If successful, resp->id
 * contains the queue ID.
 *
 * Note: resp->id contains a virtual ID if vf_request is true.
 *
 * Errors:
 * EINVAL - A requested resource is unavailable, the domain is not configured,
 *	    the domain has already been started, or the requested queue name is
 *	    already in use.
 * EFAULT - Internal error (resp->status not set).
 */
int dlb_hw_create_ldb_queue(struct dlb_hw *hw,
			    u32 domain_id,
			    struct dlb_create_ldb_queue_args *args,
			    struct dlb_cmd_response *resp,
			    bool vf_request,
			    unsigned int vf_id);

/**
 * dlb_hw_create_dir_queue() - create a directed queue
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: queue creation arguments.
 * @resp: response structure.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function creates a directed queue.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum dlb_error. If successful, resp->id
 * contains the queue ID.
 *
 * Note: resp->id contains a virtual ID if vf_request is true.
 *
 * Errors:
 * EINVAL - A requested resource is unavailable, the domain is not configured,
 *	    or the domain has already been started.
 * EFAULT - Internal error (resp->status not set).
 */
int dlb_hw_create_dir_queue(struct dlb_hw *hw,
			    u32 domain_id,
			    struct dlb_create_dir_queue_args *args,
			    struct dlb_cmd_response *resp,
			    bool vf_request,
			    unsigned int vf_id);

/**
 * dlb_hw_create_dir_port() - create a directed port
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: port creation arguments.
 * @pop_count_dma_base: base address of the pop count memory. This can be
 *			a PA or an IOVA.
 * @cq_dma_base: base address of the CQ memory. This can be a PA or an IOVA.
 * @resp: response structure.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function creates a directed port.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum dlb_error. If successful, resp->id
 * contains the port ID.
 *
 * Note: resp->id contains a virtual ID if vf_request is true.
 *
 * Errors:
 * EINVAL - A requested resource is unavailable, a credit setting is invalid, a
 *	    pool ID is invalid, a pointer address is not properly aligned, the
 *	    domain is not configured, or the domain has already been started.
 * EFAULT - Internal error (resp->status not set).
 */
int dlb_hw_create_dir_port(struct dlb_hw *hw,
			   u32 domain_id,
			   struct dlb_create_dir_port_args *args,
			   uintptr_t pop_count_dma_base,
			   uintptr_t cq_dma_base,
			   struct dlb_cmd_response *resp,
			   bool vf_request,
			   unsigned int vf_id);

/**
 * dlb_hw_create_ldb_port() - create a load-balanced port
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: port creation arguments.
 * @pop_count_dma_base: base address of the pop count memory. This can be
 *			 a PA or an IOVA.
 * @cq_dma_base: base address of the CQ memory. This can be a PA or an IOVA.
 * @resp: response structure.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function creates a load-balanced port.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum dlb_error. If successful, resp->id
 * contains the port ID.
 *
 * Note: resp->id contains a virtual ID if vf_request is true.
 *
 * Errors:
 * EINVAL - A requested resource is unavailable, a credit setting is invalid, a
 *	    pool ID is invalid, a pointer address is not properly aligned, the
 *	    domain is not configured, or the domain has already been started.
 * EFAULT - Internal error (resp->status not set).
 */
int dlb_hw_create_ldb_port(struct dlb_hw *hw,
			   u32 domain_id,
			   struct dlb_create_ldb_port_args *args,
			   uintptr_t pop_count_dma_base,
			   uintptr_t cq_dma_base,
			   struct dlb_cmd_response *resp,
			   bool vf_request,
			   unsigned int vf_id);

/**
 * dlb_reset_domain() - reset a scheduling domain
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function resets and frees a DLB scheduling domain and its associated
 * resources.
 *
 * Pre-condition: the driver must ensure software has stopped sending QEs
 * through this domain's producer ports before invoking this function, or
 * undefined behavior will result.
 *
 * Return:
 * Returns 0 upon success, -1 otherwise.
 *
 * EINVAL - Invalid domain ID, or the domain is not configured.
 * EFAULT - Internal error. (Possibly caused if software is the pre-condition
 *	    is not met.)
 * ETIMEDOUT - Hardware component didn't reset in the expected time.
 */
int dlb_reset_domain(struct dlb_hw *hw,
		     u32 domain_id,
		     bool vf_request,
		     unsigned int vf_id);

/**
 * dlb_ldb_port_owned_by_domain() - query whether a port is owned by a domain
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @port_id: indicates whether this request came from a VF.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function returns whether a load-balanced port is owned by a specified
 * domain.
 *
 * Return:
 * Returns 0 if false, 1 if true, <0 otherwise.
 *
 * EINVAL - Invalid domain or port ID, or the domain is not configured.
 */
int dlb_ldb_port_owned_by_domain(struct dlb_hw *hw,
				 u32 domain_id,
				 u32 port_id,
				 bool vf_request,
				 unsigned int vf_id);

/**
 * dlb_dir_port_owned_by_domain() - query whether a port is owned by a domain
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @port_id: indicates whether this request came from a VF.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function returns whether a directed port is owned by a specified
 * domain.
 *
 * Return:
 * Returns 0 if false, 1 if true, <0 otherwise.
 *
 * EINVAL - Invalid domain or port ID, or the domain is not configured.
 */
int dlb_dir_port_owned_by_domain(struct dlb_hw *hw,
				 u32 domain_id,
				 u32 port_id,
				 bool vf_request,
				 unsigned int vf_id);

/**
 * dlb_hw_get_num_resources() - query the PCI function's available resources
 * @arg: pointer to resource counts.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function returns the number of available resources for the PF or for a
 * VF.
 *
 * Return:
 * Returns 0 upon success, -EINVAL if vf_request is true and vf_id is invalid.
 */
int dlb_hw_get_num_resources(struct dlb_hw *hw,
			     struct dlb_get_num_resources_args *arg,
			     bool vf_request,
			     unsigned int vf_id);

/**
 * dlb_disable_dp_vasr_feature() - disable directed pipe VAS reset hardware
 * @hw: dlb_hw handle for a particular device.
 *
 * This function disables certain hardware in the directed pipe,
 * necessary to workaround a DLB VAS reset issue.
 */
void dlb_disable_dp_vasr_feature(struct dlb_hw *hw);

/**
 * dlb_hw_get_ldb_queue_depth() - returns the depth of a load-balanced queue
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: queue depth args
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function returns the depth of a load-balanced queue.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum dlb_error. If successful, resp->id
 * contains the depth.
 *
 * Errors:
 * EINVAL - Invalid domain ID or queue ID.
 */
int dlb_hw_get_ldb_queue_depth(struct dlb_hw *hw,
			       u32 domain_id,
			       struct dlb_get_ldb_queue_depth_args *args,
			       struct dlb_cmd_response *resp,
			       bool vf_request,
			       unsigned int vf_id);

/**
 * dlb_hw_get_dir_queue_depth() - returns the depth of a directed queue
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: queue depth args
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function returns the depth of a directed queue.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum dlb_error. If successful, resp->id
 * contains the depth.
 *
 * Errors:
 * EINVAL - Invalid domain ID or queue ID.
 */
int dlb_hw_get_dir_queue_depth(struct dlb_hw *hw,
			       u32 domain_id,
			       struct dlb_get_dir_queue_depth_args *args,
			       struct dlb_cmd_response *resp,
			       bool vf_request,
			       unsigned int vf_id);

/**
 * dlb_hw_enable_sparse_ldb_cq_mode() - enable sparse mode for load-balanced
 *	ports.
 * @hw: dlb_hw handle for a particular device.
 *
 * This function must be called prior to configuring scheduling domains.
 */
void dlb_hw_enable_sparse_ldb_cq_mode(struct dlb_hw *hw);

/**
 * dlb_hw_enable_sparse_dir_cq_mode() - enable sparse mode for directed ports
 * @hw: dlb_hw handle for a particular device.
 *
 * This function must be called prior to configuring scheduling domains.
 */
void dlb_hw_enable_sparse_dir_cq_mode(struct dlb_hw *hw);

#endif /* __DLB_RESOURCE_H */
