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

#endif /* __DLB_RESOURCE_H */
