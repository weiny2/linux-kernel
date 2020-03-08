/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
 * Copyright(c) 2016-2018 Intel Corporation
 */

#ifndef __DLB_RESOURCE_H
#define __DLB_RESOURCE_H

#include <linux/types.h>

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
