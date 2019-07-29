/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
 * Copyright(c) 2016-2018 Intel Corporation
 */

#ifndef __HQMV2_BASE_HQMV2_API_H
#define __HQMV2_BASE_HQMV2_API_H

#include "hqmv2_hw_types.h"
#include "hqmv2_osdep_types.h"
#include "uapi/linux/hqmv2_user.h"

/**
 * hqmv2_resource_init() - initialize the device
 * @hw: pointer to struct hqmv2_hw.
 *
 * This function initializes the device's software state (pointed to by the hw
 * argument) and programs global scheduling QoS registers. This function should
 * be called during driver initialization.
 *
 * The hqmv2_hw struct must be unique per HQMV2 device and persist until the
 * device is reset.
 *
 * Return:
 * Returns 0 upon success, -1 otherwise.
 */
int hqmv2_resource_init(struct hqmv2_hw *hw);

/**
 * hqmv2_resource_free() - free device state memory
 * @hw: hqmv2_hw handle for a particular device.
 *
 * This function frees software state pointed to by hqmv2_hw. This function
 * should be called when resetting the device or unloading the driver.
 */
void hqmv2_resource_free(struct hqmv2_hw *hw);

/**
 * hqmv2_resource_reset() - reset in-use resources to their initial state
 * @hw: hqmv2_hw handle for a particular device.
 *
 * This function resets in-use resources, and makes them available for use.
 * All resources go back to their owning function, whether a PF or a VF.
 */
void hqmv2_resource_reset(struct hqmv2_hw *hw);

/**
 * hqmv2_hw_create_sched_domain() - create a scheduling domain
 * @hw: hqmv2_hw handle for a particular device.
 * @args: scheduling domain creation arguments.
 * @resp: response structure.
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function creates a scheduling domain containing the resources specified
 * in args. The individual resources (queues, ports, credits) can be configured
 * after creating a scheduling domain.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum hqmv2_error. If successful, resp->id
 * contains the domain ID.
 *
 * Note: resp->id contains a virtual ID if vdev_request is true.
 *
 * Errors:
 * EINVAL - A requested resource is unavailable, or the requested domain name
 *	    is already in use.
 * EFAULT - Internal error (resp->status not set).
 */
int hqmv2_hw_create_sched_domain(struct hqmv2_hw *hw,
				 struct hqmv2_create_sched_domain_args *args,
				 struct hqmv2_cmd_response *resp,
				 bool vdev_request,
				 unsigned int vdev_id);

/**
 * hqmv2_hw_create_ldb_queue() - create a load-balanced queue
 * @hw: hqmv2_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: queue creation arguments.
 * @resp: response structure.
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function creates a load-balanced queue.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum hqmv2_error. If successful, resp->id
 * contains the queue ID.
 *
 * Note: resp->id contains a virtual ID if vdev_request is true.
 *
 * Errors:
 * EINVAL - A requested resource is unavailable, the domain is not configured,
 *	    the domain has already been started, or the requested queue name is
 *	    already in use.
 * EFAULT - Internal error (resp->status not set).
 */
int hqmv2_hw_create_ldb_queue(struct hqmv2_hw *hw,
			      u32 domain_id,
			      struct hqmv2_create_ldb_queue_args *args,
			      struct hqmv2_cmd_response *resp,
			      bool vdev_request,
			      unsigned int vdev_id);

/**
 * hqmv2_hw_create_dir_queue() - create a directed queue
 * @hw: hqmv2_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: queue creation arguments.
 * @resp: response structure.
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function creates a directed queue.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum hqmv2_error. If successful, resp->id
 * contains the queue ID.
 *
 * Note: resp->id contains a virtual ID if vdev_request is true.
 *
 * Errors:
 * EINVAL - A requested resource is unavailable, the domain is not configured,
 *	    or the domain has already been started.
 * EFAULT - Internal error (resp->status not set).
 */
int hqmv2_hw_create_dir_queue(struct hqmv2_hw *hw,
			      u32 domain_id,
			      struct hqmv2_create_dir_queue_args *args,
			      struct hqmv2_cmd_response *resp,
			      bool vdev_request,
			      unsigned int vdev_id);

/**
 * hqmv2_hw_create_dir_port() - create a directed port
 * @hw: hqmv2_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: port creation arguments.
 * @pop_count_dma_base: base address of the pop count memory. This can be
 *			a PA or an IOVA.
 * @cq_dma_base: base address of the CQ memory. This can be a PA or an IOVA.
 * @resp: response structure.
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function creates a directed port.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum hqmv2_error. If successful, resp->id
 * contains the port ID.
 *
 * Note: resp->id contains a virtual ID if vdev_request is true.
 *
 * Errors:
 * EINVAL - A requested resource is unavailable, a credit setting is invalid, a
 *	    pointer address is not properly aligned, the domain is not
 *	    configured, or the domain has already been started.
 * EFAULT - Internal error (resp->status not set).
 */
int hqmv2_hw_create_dir_port(struct hqmv2_hw *hw,
			     u32 domain_id,
			     struct hqmv2_create_dir_port_args *args,
			     uintptr_t pop_count_dma_base,
			     uintptr_t cq_dma_base,
			     struct hqmv2_cmd_response *resp,
			     bool vdev_request,
			     unsigned int vdev_id);

/**
 * hqmv2_hw_create_ldb_port() - create a load-balanced port
 * @hw: hqmv2_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: port creation arguments.
 * @pop_count_dma_base: base address of the pop count memory. This can be
 *			 a PA or an IOVA.
 * @cq_dma_base: base address of the CQ memory. This can be a PA or an IOVA.
 * @resp: response structure.
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function creates a load-balanced port.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum hqmv2_error. If successful, resp->id
 * contains the port ID.
 *
 * Note: resp->id contains a virtual ID if vdev_request is true.
 *
 * Errors:
 * EINVAL - A requested resource is unavailable, a credit setting is invalid, a
 *	    pointer address is not properly aligned, the domain is not
 *	    configured, or the domain has already been started.
 * EFAULT - Internal error (resp->status not set).
 */
int hqmv2_hw_create_ldb_port(struct hqmv2_hw *hw,
			     u32 domain_id,
			     struct hqmv2_create_ldb_port_args *args,
			     uintptr_t pop_count_dma_base,
			     uintptr_t cq_dma_base,
			     struct hqmv2_cmd_response *resp,
			     bool vdev_request,
			     unsigned int vdev_id);

/**
 * hqmv2_hw_start_domain() - start a scheduling domain
 * @hw: hqmv2_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: start domain arguments.
 * @resp: response structure.
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function starts a scheduling domain, which allows applications to send
 * traffic through it. Once a domain is started, its resources can no longer be
 * configured (besides QID remapping and port enable/disable).
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum hqmv2_error.
 *
 * Errors:
 * EINVAL - the domain is not configured, or the domain is already started.
 */
int hqmv2_hw_start_domain(struct hqmv2_hw *hw,
			  u32 domain_id,
			  struct hqmv2_start_domain_args *args,
			  struct hqmv2_cmd_response *resp,
			  bool vdev_request,
			  unsigned int vdev_id);

/**
 * hqmv2_hw_map_qid() - map a load-balanced queue to a load-balanced port
 * @hw: hqmv2_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: map QID arguments.
 * @resp: response structure.
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function configures the HQMV2 to schedule QEs from the specified queue
 * to the specified port. Each load-balanced port can be mapped to up to 8
 * queues; each load-balanced queue can potentially map to all the
 * load-balanced ports.
 *
 * A successful return does not necessarily mean the mapping was configured. If
 * this function is unable to immediately map the queue to the port, it will
 * add the requested operation to a per-port list of pending map/unmap
 * operations, and (if it's not already running) launch a kernel thread that
 * periodically attempts to process all pending operations. In a sense, this is
 * an asynchronous function.
 *
 * This asynchronicity creates two views of the state of hardware: the actual
 * hardware state and the requested state (as if every request completed
 * immediately). If there are any pending map/unmap operations, the requested
 * state will differ from the actual state. All validation is performed with
 * respect to the pending state; for instance, if there are 8 pending map
 * operations for port X, a request for a 9th will fail because a load-balanced
 * port can only map up to 8 queues.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum hqmv2_error.
 *
 * Errors:
 * EINVAL - A requested resource is unavailable, invalid port or queue ID, or
 *	    the domain is not configured.
 * EFAULT - Internal error (resp->status not set).
 */
int hqmv2_hw_map_qid(struct hqmv2_hw *hw,
		     u32 domain_id,
		     struct hqmv2_map_qid_args *args,
		     struct hqmv2_cmd_response *resp,
		     bool vdev_request,
		     unsigned int vdev_id);

/**
 * hqmv2_hw_unmap_qid() - Unmap a load-balanced queue from a load-balanced port
 * @hw: hqmv2_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: unmap QID arguments.
 * @resp: response structure.
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function configures the HQMV2 to stop scheduling QEs from the specified
 * queue to the specified port.
 *
 * A successful return does not necessarily mean the mapping was removed. If
 * this function is unable to immediately unmap the queue from the port, it
 * will add the requested operation to a per-port list of pending map/unmap
 * operations, and (if it's not already running) launch a kernel thread that
 * periodically attempts to process all pending operations. See
 * hqmv2_hw_map_qid() for more details.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum hqmv2_error.
 *
 * Errors:
 * EINVAL - A requested resource is unavailable, invalid port or queue ID, or
 *	    the domain is not configured.
 * EFAULT - Internal error (resp->status not set).
 */
int hqmv2_hw_unmap_qid(struct hqmv2_hw *hw,
		       u32 domain_id,
		       struct hqmv2_unmap_qid_args *args,
		       struct hqmv2_cmd_response *resp,
		       bool vdev_request,
		       unsigned int vdev_id);

/**
 * hqmv2_finish_unmap_qid_procedures() - finish any pending unmap procedures
 * @hw: hqmv2_hw handle for a particular device.
 *
 * This function attempts to finish any outstanding unmap procedures.
 * This function should be called by the kernel thread responsible for
 * finishing map/unmap procedures.
 *
 * Return:
 * Returns the number of procedures that weren't completed.
 */
unsigned int hqmv2_finish_unmap_qid_procedures(struct hqmv2_hw *hw);

/**
 * hqmv2_finish_map_qid_procedures() - finish any pending map procedures
 * @hw: hqmv2_hw handle for a particular device.
 *
 * This function attempts to finish any outstanding map procedures.
 * This function should be called by the kernel thread responsible for
 * finishing map/unmap procedures.
 *
 * Return:
 * Returns the number of procedures that weren't completed.
 */
unsigned int hqmv2_finish_map_qid_procedures(struct hqmv2_hw *hw);

/**
 * hqmv2_hw_enable_ldb_port() - enable a load-balanced port for scheduling
 * @hw: hqmv2_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: port enable arguments.
 * @resp: response structure.
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function configures the HQMV2 to schedule QEs to a load-balanced port.
 * Ports are enabled by default.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum hqmv2_error.
 *
 * Errors:
 * EINVAL - The port ID is invalid or the domain is not configured.
 * EFAULT - Internal error (resp->status not set).
 */
int hqmv2_hw_enable_ldb_port(struct hqmv2_hw *hw,
			     u32 domain_id,
			     struct hqmv2_enable_ldb_port_args *args,
			     struct hqmv2_cmd_response *resp,
			     bool vdev_request,
			     unsigned int vdev_id);

/**
 * hqmv2_hw_disable_ldb_port() - disable a load-balanced port for scheduling
 * @hw: hqmv2_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: port disable arguments.
 * @resp: response structure.
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function configures the HQMV2 to stop scheduling QEs to a load-balanced
 * port. Ports are enabled by default.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum hqmv2_error.
 *
 * Errors:
 * EINVAL - The port ID is invalid or the domain is not configured.
 * EFAULT - Internal error (resp->status not set).
 */
int hqmv2_hw_disable_ldb_port(struct hqmv2_hw *hw,
			      u32 domain_id,
			      struct hqmv2_disable_ldb_port_args *args,
			      struct hqmv2_cmd_response *resp,
			      bool vdev_request,
			      unsigned int vdev_id);

/**
 * hqmv2_hw_enable_dir_port() - enable a directed port for scheduling
 * @hw: hqmv2_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: port enable arguments.
 * @resp: response structure.
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function configures the HQMV2 to schedule QEs to a directed port.
 * Ports are enabled by default.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum hqmv2_error.
 *
 * Errors:
 * EINVAL - The port ID is invalid or the domain is not configured.
 * EFAULT - Internal error (resp->status not set).
 */
int hqmv2_hw_enable_dir_port(struct hqmv2_hw *hw,
			     u32 domain_id,
			     struct hqmv2_enable_dir_port_args *args,
			     struct hqmv2_cmd_response *resp,
			     bool vdev_request,
			     unsigned int vdev_id);

/**
 * hqmv2_hw_disable_dir_port() - disable a directed port for scheduling
 * @hw: hqmv2_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: port disable arguments.
 * @resp: response structure.
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function configures the HQMV2 to stop scheduling QEs to a directed port.
 * Ports are enabled by default.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum hqmv2_error.
 *
 * Errors:
 * EINVAL - The port ID is invalid or the domain is not configured.
 * EFAULT - Internal error (resp->status not set).
 */
int hqmv2_hw_disable_dir_port(struct hqmv2_hw *hw,
			      u32 domain_id,
			      struct hqmv2_disable_dir_port_args *args,
			      struct hqmv2_cmd_response *resp,
			      bool vdev_request,
			      unsigned int vdev_id);

/**
 * hqmv2_configure_ldb_cq_interrupt() - configure load-balanced CQ for
 *					interrupts
 * @hw: hqmv2_hw handle for a particular device.
 * @port_id: load-balancd port ID.
 * @vector: interrupt vector ID. Should be 0 for MSI or compressed MSI-X mode,
 *	    else a value up to 64.
 * @mode: interrupt type (HQMV2_CQ_ISR_MODE_MSI or HQMV2_CQ_ISR_MODE_MSIX)
 * @vf: If the port is VF-owned, the VF's ID. This is used for translating the
 *	virtual port ID to a physical port ID. Ignored if mode is not MSI.
 * @owner_vf: the VF to route the interrupt to. Ignore if mode is not MSI.
 * @threshold: the minimum CQ depth at which the interrupt can fire. Must be
 *	greater than 0.
 *
 * This function configures the HQMV2 registers for load-balanced CQ's
 * interrupts. This doesn't enable the CQ's interrupt; that can be done with
 * hqmv2_arm_cq_interrupt() or through an interrupt arm QE.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise.
 *
 * Errors:
 * EINVAL - The port ID is invalid.
 */
int hqmv2_configure_ldb_cq_interrupt(struct hqmv2_hw *hw,
				     int port_id,
				     int vector,
				     int mode,
				     unsigned int vf,
				     unsigned int owner_vf,
				     u16 threshold);

/**
 * hqmv2_configure_dir_cq_interrupt() - configure directed CQ for interrupts
 * @hw: hqmv2_hw handle for a particular device.
 * @port_id: load-balancd port ID.
 * @vector: interrupt vector ID. Should be 0 for MSI or compressed MSI-X mode,
 *	    else a value up to 64.
 * @mode: interrupt type (HQMV2_CQ_ISR_MODE_MSI or HQMV2_CQ_ISR_MODE_MSIX)
 * @vf: If the port is VF-owned, the VF's ID. This is used for translating the
 *	virtual port ID to a physical port ID. Ignored if mode is not MSI.
 * @owner_vf: the VF to route the interrupt to. Ignore if mode is not MSI.
 * @threshold: the minimum CQ depth at which the interrupt can fire. Must be
 *	greater than 0.
 *
 * This function configures the HQMV2 registers for directed CQ's interrupts.
 * This doesn't enable the CQ's interrupt; that can be done with
 * hqmv2_arm_cq_interrupt() or through an interrupt arm QE.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise.
 *
 * Errors:
 * EINVAL - The port ID is invalid.
 */
int hqmv2_configure_dir_cq_interrupt(struct hqmv2_hw *hw,
				     int port_id,
				     int vector,
				     int mode,
				     unsigned int vf,
				     unsigned int owner_vf,
				     u16 threshold);

/**
 * hqmv2_enable_alarm_interrupts() - enable certain hardware alarm interrupts
 * @hw: hqmv2_hw handle for a particular device.
 *
 * This function configures the following HQMV2 alarms:
 * the DM address translation alarm, NQ full alarm, and the ingress error
 * alarm. (Other alarms are enabled by default.)
 */
void hqmv2_enable_alarm_interrupts(struct hqmv2_hw *hw);

/**
 * hqmv2_set_msix_mode() - enable certain hardware alarm interrupts
 * @hw: hqmv2_hw handle for a particular device.
 * @mode: MSI-X mode (HQMV2_MSIX_MODE_PACKED or HQMV2_MSIX_MODE_COMPRESSED)
 *
 * This function configures the hardware to use either packed or compressed
 * mode. This function should not be called if using MSI interrupts.
 */
void hqmv2_set_msix_mode(struct hqmv2_hw *hw, int mode);

/**
 * hqmv2_handle_service_intr() - handle a service interrupt
 * @hw: hqmv2_hw handle for a particular device.
 * @mbox_hdlr: callback function for handling mailbox requests.
 *
 * This function handles a service interrupt, which could be caused by any
 * combination of: hardware alarm, watchdog timer, mailbox message, and ingress
 * error.
 */
void hqmv2_handle_service_intr(struct hqmv2_hw *hw,
			       void (*mbox_hdlr)(struct hqmv2_hw *));

/**
 * hqmv2_cq_depth() - query a CQ's depth
 * @hw: hqmv2_hw handle for a particular device.
 * @port_id: port ID
 * @is_ldb: true for load-balanced port, false for a directed port
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function returns the CQ's current depth. It can be used in CQ interrupt
 * code to check if the caller needs to block (depth == 0), or as a thread
 * wake-up condition (depth != 0).
 *
 * The function does no parameter validation; that is the caller's
 * responsibility.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return: returns the CQ's depth (>= 0), or an error otherwise.
 *
 * EINVAL - Invalid port ID.
 */
int hqmv2_cq_depth(struct hqmv2_hw *hw,
		   int port_id,
		   bool is_ldb,
		   bool vdev_request,
		   unsigned int vdev_id);

/**
 * hqmv2_arm_cq_interrupt() - arm a CQ's interrupt
 * @hw: hqmv2_hw handle for a particular device.
 * @port_id: port ID
 * @is_ldb: true for load-balanced port, false for a directed port
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function arms the CQ's interrupt. The CQ must be configured prior to
 * calling this function.
 *
 * The function does no parameter validation; that is the caller's
 * responsibility.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return: returns 0 upon success, <0 otherwise.
 *
 * EINVAL - Invalid port ID.
 */
int hqmv2_arm_cq_interrupt(struct hqmv2_hw *hw,
			   int port_id,
			   bool is_ldb,
			   bool vdev_request,
			   unsigned int vdev_id);

/**
 * hqmv2_read_compressed_cq_intr_status() - read compressed CQ interrupt status
 * @hw: hqmv2_hw handle for a particular device.
 * @ldb_interrupts: 2-entry array of u32 bitmaps
 * @dir_interrupts: 4-entry array of u32 bitmaps
 *
 * This function can be called from a compressed CQ interrupt handler to
 * determine which CQ interrupts have fired. The caller should take appropriate
 * (such as waking threads blocked on a CQ's interrupt) then ack the interrupts
 * with hqmv2_ack_compressed_cq_intr().
 */
void hqmv2_read_compressed_cq_intr_status(struct hqmv2_hw *hw,
					  u32 *ldb_interrupts,
					  u32 *dir_interrupts);

/**
 * hqmv2_ack_compressed_cq_intr_status() - ack compressed CQ interrupts
 * @hw: hqmv2_hw handle for a particular device.
 * @ldb_interrupts: 2-entry array of u32 bitmaps
 * @dir_interrupts: 4-entry array of u32 bitmaps
 *
 * This function ACKs compressed CQ interrupts. Its arguments should be the
 * same ones passed to hqmv2_read_compressed_cq_intr_status().
 */
void hqmv2_ack_compressed_cq_intr(struct hqmv2_hw *hw,
				  u32 *ldb_interrupts,
				  u32 *dir_interrupts);

/**
 * hqmv2_read_vf_intr_status() - read the VF interrupt status register
 * @hw: hqmv2_hw handle for a particular device.
 *
 * This function can be called from a VF's interrupt handler to determine
 * which interrupts have fired. The first 31 bits correspond to CQ interrupt
 * vectors, and the final bit is for the PF->VF mailbox interrupt vector.
 *
 * Return:
 * Returns a bit vector indicating which interrupt vectors are active.
 */
u32 hqmv2_read_vf_intr_status(struct hqmv2_hw *hw);

/**
 * hqmv2_ack_vf_intr_status() - ack VF interrupts
 * @hw: hqmv2_hw handle for a particular device.
 * @interrupts: 32-bit bitmap
 *
 * This function ACKs a VF's interrupts. Its interrupts argument should be the
 * value returned by hqmv2_read_vf_intr_status().
 */
void hqmv2_ack_vf_intr_status(struct hqmv2_hw *hw, u32 interrupts);

/**
 * hqmv2_ack_vf_msi_intr() - ack VF MSI interrupt
 * @hw: hqmv2_hw handle for a particular device.
 * @interrupts: 32-bit bitmap
 *
 * This function clears the VF's MSI interrupt pending register. Its interrupts
 * argument should be contain the MSI vectors to ACK. For example, if MSI MME
 * is in mode 0, then one bit 0 should ever be set.
 */
void hqmv2_ack_vf_msi_intr(struct hqmv2_hw *hw, u32 interrupts);

/**
 * hqmv2_ack_pf_mbox_int() - ack PF->VF mailbox interrupt
 * @hw: hqmv2_hw handle for a particular device.
 *
 * When done processing the PF mailbox request, this function unsets
 * the PF's mailbox ISR register.
 */
void hqmv2_ack_pf_mbox_int(struct hqmv2_hw *hw);

/**
 * hqmv2_process_alarm_interrupt() - process an alarm interrupt
 * @hw: hqmv2_hw handle for a particular device.
 *
 * This function reads the alarm syndrome, logs its, and acks the interrupt.
 * This function should be called from the alarm interrupt handler when
 * interrupt vector HQMV2_INT_ALARM fires.
 */
void hqmv2_process_alarm_interrupt(struct hqmv2_hw *hw);

/**
 * hqmv2_read_vdev_to_pf_int_bitvec() - return a bit vector of all requesting
 *					vdevs
 * @hw: hqmv2_hw handle for a particular device.
 *
 * When the vdev->PF ISR fires, this function can be called to determine which
 * vdev(s) are requesting service. This bitvector must be passed to
 * hqmv2_ack_vdev_to_pf_int() when processing is complete for all requesting
 * vdevs.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns a bit vector indicating which VFs (0-15) have requested service.
 */
u32 hqmv2_read_vdev_to_pf_int_bitvec(struct hqmv2_hw *hw);

/**
 * hqmv2_ack_vdev_mbox_int() - ack processed vdev->PF mailbox interrupt
 * @hw: hqmv2_hw handle for a particular device.
 * @bitvec: bit vector returned by hqmv2_read_vdev_to_pf_int_bitvec()
 *
 * When done processing all VF mailbox requests, this function unsets the VF's
 * mailbox ISR register.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 */
void hqmv2_ack_vdev_mbox_int(struct hqmv2_hw *hw, u32 bitvec);

/**
 * hqmv2_read_vf_flr_int_bitvec() - return a bit vector of all VFs requesting
 *				    FLR
 * @hw: hqmv2_hw handle for a particular device.
 *
 * When the VF FLR ISR fires, this function can be called to determine which
 * VF(s) are requesting FLRs. This bitvector must passed to
 * hqmv2_ack_vf_flr_int() when processing is complete for all requesting VFs.
 *
 * Return:
 * Returns a bit vector indicating which VFs (0-15) have requested FLRs.
 */
u32 hqmv2_read_vf_flr_int_bitvec(struct hqmv2_hw *hw);

/**
 * hqmv2_ack_vf_flr_int() - ack processed VF<->PF interrupt(s)
 * @hw: hqmv2_hw handle for a particular device.
 * @bitvec: bit vector returned by hqmv2_read_vf_flr_int_bitvec()
 *
 * When done processing all VF FLR requests, this function unsets the VF's FLR
 * ISR register.
 */
void hqmv2_ack_vf_flr_int(struct hqmv2_hw *hw, u32 bitvec);

/**
 * hqmv2_ack_vdev_to_pf_int() - ack processed VF mbox and FLR interrupt(s)
 * @hw: hqmv2_hw handle for a particular device.
 * @mbox_bitvec: bit vector returned by hqmv2_read_vdev_to_pf_int_bitvec()
 * @flr_bitvec: bit vector returned by hqmv2_read_vf_flr_int_bitvec()
 *
 * When done processing all VF requests, this function communicates to the
 * hardware that processing is complete.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 */
void hqmv2_ack_vdev_to_pf_int(struct hqmv2_hw *hw,
			      u32 mbox_bitvec,
			      u32 flr_bitvec);

/**
 * hqmv2_process_wdt_interrupt() - process watchdog timer interrupts
 * @hw: hqmv2_hw handle for a particular device.
 *
 * FIXME: Describe when implemented
 */
void hqmv2_process_wdt_interrupt(struct hqmv2_hw *hw);

/**
 * hqmv2_process_ingress_error_interrupt() - process ingress error interrupts
 * @hw: hqmv2_hw handle for a particular device.
 *
 * This function reads the alarm syndrome, logs it, notifies user-space, and
 * acks the interrupt. This function should be called from the alarm interrupt
 * handler when interrupt vector HQMV2_INT_INGRESS_ERROR fires.
 */
void hqmv2_process_ingress_error_interrupt(struct hqmv2_hw *hw);

/**
 * hqmv2_get_group_sequence_numbers() - return a group's number of SNs per queue
 * @hw: hqmv2_hw handle for a particular device.
 * @group_id: sequence number group ID.
 *
 * This function returns the configured number of sequence numbers per queue
 * for the specified group.
 *
 * Return:
 * Returns -EINVAL if group_id is invalid, else the group's SNs per queue.
 */
int hqmv2_get_group_sequence_numbers(struct hqmv2_hw *hw,
				     unsigned int group_id);

/**
 * hqmv2_set_group_sequence_numbers() - assign a group's number of SNs per queue
 * @hw: hqmv2_hw handle for a particular device.
 * @group_id: sequence number group ID.
 * @val: requested amount of sequence numbers per queue.
 *
 * This function configures the group's number of sequence numbers per queue.
 * val can be a power-of-two between 32 and 1024, inclusive. This setting can
 * be configured until the first ordered load-balanced queue is configured, at
 * which point the configuration is locked.
 *
 * Return:
 * Returns 0 upon success; -EINVAL if group_id or val is invalid, -EPERM if an
 * ordered queue is configured.
 */
int hqmv2_set_group_sequence_numbers(struct hqmv2_hw *hw,
				     unsigned int group_id,
				     unsigned long val);

/**
 * hqmv2_reset_domain() - reset a scheduling domain
 * @hw: hqmv2_hw handle for a particular device.
 * @domain_id: domain ID.
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function resets and frees an HQMV2 scheduling domain and its associated
 * resources.
 *
 * Pre-condition: the driver must ensure software has stopped sending QEs
 * through this domain's producer ports before invoking this function, or
 * undefined behavior will result.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, -1 otherwise.
 *
 * EINVAL - Invalid domain ID, or the domain is not configured.
 * EFAULT - Internal error. (Possibly caused if software is the pre-condition
 *	    is not met.)
 * ETIMEDOUT - Hardware component didn't reset in the expected time.
 */
int hqmv2_reset_domain(struct hqmv2_hw *hw,
		       u32 domain_id,
		       bool vdev_request,
		       unsigned int vdev_id);

/**
 * hqmv2_ldb_port_owned_by_domain() - query whether a port is owned by a domain
 * @hw: hqmv2_hw handle for a particular device.
 * @domain_id: domain ID.
 * @port_id: indicates whether this request came from a VF.
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function returns whether a load-balanced port is owned by a specified
 * domain.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 if false, 1 if true, <0 otherwise.
 *
 * EINVAL - Invalid domain or port ID, or the domain is not configured.
 */
int hqmv2_ldb_port_owned_by_domain(struct hqmv2_hw *hw,
				   u32 domain_id,
				   u32 port_id,
				   bool vdev_request,
				   unsigned int vdev_id);

/**
 * hqmv2_dir_port_owned_by_domain() - query whether a port is owned by a domain
 * @hw: hqmv2_hw handle for a particular device.
 * @domain_id: domain ID.
 * @port_id: indicates whether this request came from a VF.
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function returns whether a directed port is owned by a specified
 * domain.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 if false, 1 if true, <0 otherwise.
 *
 * EINVAL - Invalid domain or port ID, or the domain is not configured.
 */
int hqmv2_dir_port_owned_by_domain(struct hqmv2_hw *hw,
				   u32 domain_id,
				   u32 port_id,
				   bool vdev_request,
				   unsigned int vdev_id);

/**
 * hqmv2_hw_get_num_resources() - query the PCI function's available resources
 * @arg: pointer to resource counts.
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function returns the number of available resources for the PF or for a
 * VF.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, -1 if vdev_request is true and vdev_id is invalid.
 */
int hqmv2_hw_get_num_resources(struct hqmv2_hw *hw,
			       struct hqmv2_get_num_resources_args *arg,
			       bool vdev_request,
			       unsigned int vdev_id);

/**
 * hqmv2_hw_get_num_used_resources() - query the PCI function's used resources
 * @arg: pointer to resource counts.
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function returns the number of resources in use by the PF or a VF. It
 * fills in the fields that args points to, except the following:
 * - max_contiguous_atomic_inflights
 * - max_contiguous_hist_list_entries
 * - max_contiguous_ldb_credits
 * - max_contiguous_dir_credits
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, -1 if vdev_request is true and vdev_id is invalid.
 */
int hqmv2_hw_get_num_used_resources(struct hqmv2_hw *hw,
				    struct hqmv2_get_num_resources_args *arg,
				    bool vdev_request,
				    unsigned int vdev_id);

/* For free-running counters, we record the initial data at setup time */
struct hqmv2_perf_metric_pre_data {
	u64 ldb_sched_count;
	u64 dir_sched_count;
	u64 ldb_cq_sched_count[HQMV2_MAX_NUM_LDB_PORTS];
	u64 dir_cq_sched_count[HQMV2_MAX_NUM_DIR_PORTS];
	u64 dm_hbm_alloc_fail_count;
	u64 mem_latency_count;
	u64 mem_latency_events;
	u64 dm_busy_count;
	u64 dm_job_completion_count;
	u64 dm_nq_full_count;
};

/**
 * hqmv2_init_perf_metric_measurement() - initialize perf metric h/w counters
 * @hw: hqmv2_hw handle for a particular device.
 * @id: perf metric group ID.
 * @duration_us: measurement duration (microseconds).
 * @pre: initial measurement counts (output arg only used by metric group 0).
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function starts the performance measurement hardware for the requested
 * performance metric group.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Note: Only one metric group can be measured at a time. Calling this function
 * successively without calling hqmv2_collect_perf_metric_data() will halt the
 * first measurement.
 */
void hqmv2_init_perf_metric_measurement(struct hqmv2_hw *hw,
					u32 id,
					u32 duration_us,
					struct hqmv2_perf_metric_pre_data *pre,
					bool vdev_request,
					unsigned int vdev_id);

/**
 * hqmv2_init_perf_metric_data() - collect perf metric h/w counter results
 * @hw: hqmv2_hw handle for a particular device.
 * @id: perf metric group ID.
 * @duration_us: measurement duration (microseconds).
 * @pre: initial measurement counts.
 * @data: measurement results (output argument).
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function collected the performance measurement data. For metric group
 * 0, it uses the initial data (the 'pre' argument) from
 * hqmv2_init_perf_metric_measurement() to calculate a delta.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 */
void hqmv2_collect_perf_metric_data(struct hqmv2_hw *hw,
				    u32 id,
				    u32 duration_us,
				    struct hqmv2_perf_metric_pre_data *pre,
				    union hqmv2_perf_metric_group_data *data,
				    bool vdev_request,
				    unsigned int vdev_id);

/**
 * hqmv2_send_async_vdev_to_pf_msg() - (vdev only) send a mailbox message to
 *				       the PF
 * @hw: hqmv2_hw handle for a particular device.
 *
 * This function sends a VF->PF mailbox message. It is asynchronous, so it
 * returns once the message is sent but potentially before the PF has processed
 * the message. The caller must call hqmv2_vdev_to_pf_complete() to determine
 * when the PF has finished processing the request.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 */
void hqmv2_send_async_vdev_to_pf_msg(struct hqmv2_hw *hw);

/**
 * hqmv2_vdev_to_pf_complete() - check the status of an asynchronous mailbox
 *				 request
 * @hw: hqmv2_hw handle for a particular device.
 *
 * This function returns a boolean indicating whether the PF has finished
 * processing a VF->PF mailbox request. It should only be called after sending
 * an asynchronous request with hqmv2_send_async_vdev_to_pf_msg().
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 */
bool hqmv2_vdev_to_pf_complete(struct hqmv2_hw *hw);

/**
 * hqmv2_vf_flr_complete() - check the status of a VF FLR
 * @hw: hqmv2_hw handle for a particular device.
 *
 * This function returns a boolean indicating whether the PF has finished
 * executing the VF FLR. It should only be called after setting the VF's FLR
 * bit.
 */
bool hqmv2_vf_flr_complete(struct hqmv2_hw *hw);

/**
 * hqmv2_send_async_pf_to_vdev_msg() - (PF only) send a mailbox message to a
 *					vdev
 * @hw: hqmv2_hw handle for a particular device.
 * @vdev_id: vdev ID.
 *
 * This function sends a PF->vdev mailbox message. It is asynchronous, so it
 * returns once the message is sent but potentially before the vdev has
 * processed the message. The caller must call hqmv2_pf_to_vdev_complete() to
 * determine when the vdev has finished processing the request.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 */
void hqmv2_send_async_pf_to_vdev_msg(struct hqmv2_hw *hw, unsigned int vdev_id);

/**
 * hqmv2_pf_to_vdev_complete() - check the status of an asynchronous mailbox
 *			       request
 * @hw: hqmv2_hw handle for a particular device.
 * @vdev_id: vdev ID.
 *
 * This function returns a boolean indicating whether the vdev has finished
 * processing a PF->vdev mailbox request. It should only be called after
 * sending an asynchronous request with hqmv2_send_async_pf_to_vdev_msg().
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 */
bool hqmv2_pf_to_vdev_complete(struct hqmv2_hw *hw, unsigned int vdev_id);

/**
 * hqmv2_pf_read_vf_mbox_req() - (PF only) read a VF->PF mailbox request
 * @hw: hqmv2_hw handle for a particular device.
 * @vf_id: VF ID.
 * @data: pointer to message data.
 * @len: size, in bytes, of the data array.
 *
 * This function copies one of the PF's VF->PF mailboxes into the array pointed
 * to by data.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * EINVAL - len >= HQMV2_VF2PF_REQ_BYTES.
 */
int hqmv2_pf_read_vf_mbox_req(struct hqmv2_hw *hw,
			      unsigned int vf_id,
			      void *data,
			      int len);

/**
 * hqmv2_pf_read_vf_mbox_resp() - (PF only) read a VF->PF mailbox response
 * @hw: hqmv2_hw handle for a particular device.
 * @vf_id: VF ID.
 * @data: pointer to message data.
 * @len: size, in bytes, of the data array.
 *
 * This function copies one of the PF's VF->PF mailboxes into the array pointed
 * to by data.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * EINVAL - len >= HQMV2_VF2PF_RESP_BYTES.
 */
int hqmv2_pf_read_vf_mbox_resp(struct hqmv2_hw *hw,
			       unsigned int vf_id,
			       void *data,
			       int len);

/**
 * hqmv2_pf_write_vf_mbox_resp() - (PF only) write a PF->VF mailbox response
 * @hw: hqmv2_hw handle for a particular device.
 * @vf_id: VF ID.
 * @data: pointer to message data.
 * @len: size, in bytes, of the data array.
 *
 * This function copies the user-provided message data into of the PF's VF->PF
 * mailboxes.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * EINVAL - len >= HQMV2_PF2VF_RESP_BYTES.
 */
int hqmv2_pf_write_vf_mbox_resp(struct hqmv2_hw *hw,
				unsigned int vf_id,
				void *data,
				int len);

/**
 * hqmv2_pf_write_vf_mbox_req() - (PF only) write a PF->VF mailbox request
 * @hw: hqmv2_hw handle for a particular device.
 * @vf_id: VF ID.
 * @data: pointer to message data.
 * @len: size, in bytes, of the data array.
 *
 * This function copies the user-provided message data into of the PF's VF->PF
 * mailboxes.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * EINVAL - len >= HQMV2_PF2VF_REQ_BYTES.
 */
int hqmv2_pf_write_vf_mbox_req(struct hqmv2_hw *hw,
			       unsigned int vf_id,
			       void *data,
			       int len);

/**
 * hqmv2_vf_read_pf_mbox_resp() - (VF only) read a PF->VF mailbox response
 * @hw: hqmv2_hw handle for a particular device.
 * @data: pointer to message data.
 * @len: size, in bytes, of the data array.
 *
 * This function copies the VF's PF->VF mailbox into the array pointed to by
 * data.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * EINVAL - len >= HQMV2_PF2VF_RESP_BYTES.
 */
int hqmv2_vf_read_pf_mbox_resp(struct hqmv2_hw *hw, void *data, int len);

/**
 * hqmv2_vf_read_pf_mbox_req() - (VF only) read a PF->VF mailbox request
 * @hw: hqmv2_hw handle for a particular device.
 * @data: pointer to message data.
 * @len: size, in bytes, of the data array.
 *
 * This function copies the VF's PF->VF mailbox into the array pointed to by
 * data.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * EINVAL - len >= HQMV2_PF2VF_REQ_BYTES.
 */
int hqmv2_vf_read_pf_mbox_req(struct hqmv2_hw *hw, void *data, int len);

/**
 * hqmv2_vf_write_pf_mbox_req() - (VF only) write a VF->PF mailbox request
 * @hw: hqmv2_hw handle for a particular device.
 * @data: pointer to message data.
 * @len: size, in bytes, of the data array.
 *
 * This function copies the user-provided message data into of the VF's PF->VF
 * mailboxes.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * EINVAL - len >= HQMV2_VF2PF_REQ_BYTES.
 */
int hqmv2_vf_write_pf_mbox_req(struct hqmv2_hw *hw, void *data, int len);

/**
 * hqmv2_vf_write_pf_mbox_resp() - (VF only) write a VF->PF mailbox response
 * @hw: hqmv2_hw handle for a particular device.
 * @data: pointer to message data.
 * @len: size, in bytes, of the data array.
 *
 * This function copies the user-provided message data into of the VF's PF->VF
 * mailboxes.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * EINVAL - len >= HQMV2_VF2PF_RESP_BYTES.
 */
int hqmv2_vf_write_pf_mbox_resp(struct hqmv2_hw *hw, void *data, int len);

/**
 * hqmv2_reset_vdev() - reset the hardware owned by a virtual device
 * @hw: hqmv2_hw handle for a particular device.
 * @id: virtual device ID
 *
 * This function resets the hardware owned by a vdev, by resetting the vdev's
 * domains one by one.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 */
int hqmv2_reset_vdev(struct hqmv2_hw *hw, unsigned int id);

/**
 * hqmv2_vdev_is_locked() - check whether the vdev's resources are locked
 * @hw: hqmv2_hw handle for a particular device.
 * @id: virtual device ID
 *
 * This function returns whether or not the vdev's resource assignments are
 * locked. If locked, no resources can be added to or subtracted from the
 * group.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 */
bool hqmv2_vdev_is_locked(struct hqmv2_hw *hw, unsigned int id);

/**
 * hqmv2_lock_vdev() - lock the vdev's resources
 * @hw: hqmv2_hw handle for a particular device.
 * @id: virtual device ID
 *
 * This function sets a flag indicating that the vdev is using its resources.
 * When the vdev is locked, its resource assignment cannot be changed.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 */
void hqmv2_lock_vdev(struct hqmv2_hw *hw, unsigned int id);

/**
 * hqmv2_unlock_vdev() - unlock the vdev's resources
 * @hw: hqmv2_hw handle for a particular device.
 * @id: virtual device ID
 *
 * This function unlocks the vdev's resource assignment, allowing it to be
 * modified.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 */
void hqmv2_unlock_vdev(struct hqmv2_hw *hw, unsigned int id);

/**
 * hqmv2_update_vdev_sched_domains() - update the domains assigned to a vdev
 * @hw: hqmv2_hw handle for a particular device.
 * @id: virtual device ID
 * @num: number of scheduling domains to assign to this vdev
 *
 * This function assigns num scheduling domains to the specified vdev. If the
 * vdev already has domains assigned, this existing assignment is adjusted
 * accordingly.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The vdev's resource assignment is locked and cannot be changed.
 */
int hqmv2_update_vdev_sched_domains(struct hqmv2_hw *hw, u32 id, u32 num);

/**
 * hqmv2_update_vdev_ldb_queues() - update the LDB queues assigned to a vdev
 * @hw: hqmv2_hw handle for a particular device.
 * @id: virtual device ID
 * @num: number of LDB queues to assign to this vdev
 *
 * This function assigns num LDB queues to the specified vdev. If the vdev
 * already has LDB queues assigned, this existing assignment is adjusted
 * accordingly.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The vdev's resource assignment is locked and cannot be changed.
 */
int hqmv2_update_vdev_ldb_queues(struct hqmv2_hw *hw, u32 id, u32 num);

/**
 * hqmv2_update_vdev_ldb_ports() - update the LDB ports assigned to a vdev
 * @hw: hqmv2_hw handle for a particular device.
 * @id: virtual device ID
 * @num: number of LDB ports to assign to this vdev
 *
 * This function assigns num LDB ports to the specified vdev. If the vdev
 * already has LDB ports assigned, this existing assignment is adjusted
 * accordingly.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The vdev's resource assignment is locked and cannot be changed.
 */
int hqmv2_update_vdev_ldb_ports(struct hqmv2_hw *hw, u32 id, u32 num);

/**
 * hqmv2_update_vdev_ldb_cos_ports() - update the LDB ports assigned to a vdev
 * @hw: hqmv2_hw handle for a particular device.
 * @id: virtual device ID
 * @cos: class-of-service ID
 * @num: number of LDB ports to assign to this vdev
 *
 * This function assigns num LDB ports from class-of-service cos to the
 * specified vdev. If the vdev already has LDB ports from this class-of-service
 * assigned, this existing assignment is adjusted accordingly.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The vdev's resource assignment is locked and cannot be changed.
 */
int hqmv2_update_vdev_ldb_cos_ports(struct hqmv2_hw *hw,
				    u32 id,
				    u32 cos,
				    u32 num);

/**
 * hqmv2_update_vdev_dir_ports() - update the DIR ports assigned to a vdev
 * @hw: hqmv2_hw handle for a particular device.
 * @id: virtual device ID
 * @num: number of DIR ports to assign to this vdev
 *
 * This function assigns num DIR ports to the specified vdev. If the vdev
 * already has DIR ports assigned, this existing assignment is adjusted
 * accordingly.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The vdev's resource assignment is locked and cannot be changed.
 */
int hqmv2_update_vdev_dir_ports(struct hqmv2_hw *hw, u32 id, u32 num);

/**
 * hqmv2_update_vdev_ldb_credits() - update the vdev's assigned LDB credits
 * @hw: hqmv2_hw handle for a particular device.
 * @id: virtual device ID
 * @num: number of LDB credit credits to assign to this vdev
 *
 * This function assigns num LDB credit to the specified vdev. If the vdev
 * already has LDB credits assigned, this existing assignment is adjusted
 * accordingly. vdevs are assigned a contiguous chunk of credits, so this
 * function may fail if a sufficiently large contiguous chunk is not available.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The vdev's resource assignment is locked and cannot be changed.
 */
int hqmv2_update_vdev_ldb_credits(struct hqmv2_hw *hw, u32 id, u32 num);

/**
 * hqmv2_update_vdev_dir_credits() - update the vdev's assigned DIR credits
 * @hw: hqmv2_hw handle for a particular device.
 * @id: virtual device ID
 * @num: number of DIR credits to assign to this vdev
 *
 * This function assigns num DIR credit to the specified vdev. If the vdev
 * already has DIR credits assigned, this existing assignment is adjusted
 * accordingly. vdevs are assigned a contiguous chunk of credits, so this
 * function may fail if a sufficiently large contiguous chunk is not available.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The vdev's resource assignment is locked and cannot be changed.
 */
int hqmv2_update_vdev_dir_credits(struct hqmv2_hw *hw, u32 id, u32 num);

/**
 * hqmv2_update_vdev_hist_list_entries() - update the vdev's assigned HL entries
 * @hw: hqmv2_hw handle for a particular device.
 * @id: virtual device ID
 * @num: number of history list entries to assign to this vdev
 *
 * This function assigns num history list entries to the specified vdev. If the
 * vdev already has history list entries assigned, this existing assignment is
 * adjusted accordingly. vdevs are assigned a contiguous chunk of entries, so
 * this function may fail if a sufficiently large contiguous chunk is not
 * available.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The vdev's resource assignment is locked and cannot be changed.
 */
int hqmv2_update_vdev_hist_list_entries(struct hqmv2_hw *hw, u32 id, u32 num);

/**
 * hqmv2_update_vdev_atomic_inflights() - update the vdev's atomic inflights
 * @hw: hqmv2_hw handle for a particular device.
 * @id: virtual device ID
 * @num: number of atomic inflights to assign to this vdev
 *
 * This function assigns num atomic inflights to the specified vdev. If the vdev
 * already has atomic inflights assigned, this existing assignment is adjusted
 * accordingly. vdevs are assigned a contiguous chunk of entries, so this
 * function may fail if a sufficiently large contiguous chunk is not available.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The vdev's resource assignment is locked and cannot be changed.
 */
int hqmv2_update_vdev_atomic_inflights(struct hqmv2_hw *hw, u32 id, u32 num);

/**
 * hqmv2_reset_vdev_resources() - reassign the vdev's resources to the PF
 * @hw: hqmv2_hw handle for a particular device.
 * @id: virtual device ID
 *
 * This function takes any resources currently assigned to the vdev and
 * reassigns them to the PF.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - id is invalid
 * EPERM  - The vdev's resource assignment is locked and cannot be changed.
 */
int hqmv2_reset_vdev_resources(struct hqmv2_hw *hw, unsigned int id);

/**
 * hqmv2_notify_vf() - send an alarm to a VF
 * @hw: hqmv2_hw handle for a particular device.
 * @vf_id: VF ID
 * @notification: notification
 *
 * This function sends a notification (as defined in hqmv2_mbox.h) to a VF.
 *
 * Return:
 * Returns 0 upon success, -1 if the VF doesn't ACK the PF->VF interrupt.
 */
int hqmv2_notify_vf(struct hqmv2_hw *hw,
		    unsigned int vf_id,
		    u32 notification);

/**
 * hqmv2_vdev_in_use() - query whether a virtual device is in use
 * @hw: hqmv2_hw handle for a particular device.
 * @id: virtual device ID
 *
 * This function sends a mailbox request to the vdev to query whether the vdev
 * is in use.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 for false, 1 for true, and -1 if the mailbox request times out or
 * an internal error occurs.
 */
int hqmv2_vdev_in_use(struct hqmv2_hw *hw, unsigned int id);

/**
 * hqmv2_clr_pmcsr_disable() - power on bulk of HQMV2 logic
 * @hw: hqmv2_hw handle for a particular device.
 *
 * Clearing the PMCSR must be done at initialization to make the device fully
 * operational.
 */
void hqmv2_clr_pmcsr_disable(struct hqmv2_hw *hw);

/**
 * hqmv2_hw_get_ldb_queue_depth() - returns the depth of a load-balanced queue
 * @hw: hqmv2_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: queue depth args
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function returns the depth of a load-balanced queue.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum hqmv2_error. If successful, resp->id
 * contains the depth.
 *
 * Errors:
 * EINVAL - Invalid domain ID or queue ID.
 */
int hqmv2_hw_get_ldb_queue_depth(struct hqmv2_hw *hw,
				 u32 domain_id,
				 struct hqmv2_get_ldb_queue_depth_args *args,
				 struct hqmv2_cmd_response *resp,
				 bool vdev_request,
				 unsigned int vdev_id);

/**
 * hqmv2_hw_get_dir_queue_depth() - returns the depth of a directed queue
 * @hw: hqmv2_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: queue depth args
 * @vdev_request: indicates whether this request came from a vdev.
 * @vdev_id: If vdev_request is true, this contains the vdev's ID.
 *
 * This function returns the depth of a directed queue.
 *
 * Note: a vdev can be either an SR-IOV virtual function or an S-IOV virtual
 *	 device.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum hqmv2_error. If successful, resp->id
 * contains the depth.
 *
 * Errors:
 * EINVAL - Invalid domain ID or queue ID.
 */
int hqmv2_hw_get_dir_queue_depth(struct hqmv2_hw *hw,
				 u32 domain_id,
				 struct hqmv2_get_dir_queue_depth_args *args,
				 struct hqmv2_cmd_response *resp,
				 bool vdev_request,
				 unsigned int vdev_id);

enum hqmv2_virt_mode {
	HQMV2_VIRT_NONE,
	HQMV2_VIRT_SRIOV,
	HQMV2_VIRT_SIOV,

	/* NUM_HQMV2_VIRT_MODES must be last */
	NUM_HQMV2_VIRT_MODES,
};

/**
 * hqmv2_hw_set_virt_mode() - set the device's virtualization mode
 * @hw: hqmv2_hw handle for a particular device.
 * @mode: either none, SR-IOV, or S-IOV.
 *
 * This function sets the virtualization mode of the device. This controls
 * whether the device uses a software or hardware mailbox.
 *
 * This should be called by the PF driver when either SR-IOV or S-IOV is
 * selected as the virtualization mechanism, and by the VF/VDEV driver during
 * initialization after recognizing itself as an SR-IOV or S-IOV device.
 *
 * Errors:
 * EINVAL - Invalid mode.
 */
int hqmv2_hw_set_virt_mode(struct hqmv2_hw *hw, enum hqmv2_virt_mode mode);

/**
 * hqmv2_hw_get_virt_mode() - get the device's virtualization mode
 * @hw: hqmv2_hw handle for a particular device.
 * @mode: either none, SR-IOV, or S-IOV.
 *
 * This function gets the virtualization mode of the device.
 */
enum hqmv2_virt_mode hqmv2_hw_get_virt_mode(struct hqmv2_hw *hw);

/**
 * hqmv2_hw_get_ldb_port_phys_id() - get a physical port ID from its virt ID
 * @hw: hqmv2_hw handle for a particular device.
 * @id: virtual port ID.
 * @vdev_id: vdev ID.
 *
 * Return:
 * Returns >= 0 upon success, -1 otherwise.
 */
s32 hqmv2_hw_get_ldb_port_phys_id(struct hqmv2_hw *hw,
				  u32 id,
				  unsigned int vdev_id);

/**
 * hqmv2_hw_get_dir_port_phys_id() - get a physical port ID from its virt ID
 * @hw: hqmv2_hw handle for a particular device.
 * @id: virtual port ID.
 * @vdev_id: vdev ID.
 *
 * Return:
 * Returns >= 0 upon success, -1 otherwise.
 */
s32 hqmv2_hw_get_dir_port_phys_id(struct hqmv2_hw *hw,
				  u32 id,
				  unsigned int vdev_id);

/**
 * hqmv2_hw_register_sw_mbox() - register a software mailbox
 * @hw: hqmv2_hw handle for a particular device.
 * @vdev_id: vdev ID.
 * @vdev2pf_mbox: pointer to a 4KB memory page used for vdev->PF communication.
 * @pf2vdev_mbox: pointer to a 4KB memory page used for PF->vdev communication.
 * @pf2vdev_inject: callback function for injecting a PF->vdev interrupt.
 *
 * When S-IOV is enabled, the VDCM must register a software mailbox for every
 * virtual device during vdev creation.
 *
 * This function notifies the driver to use a software mailbox using the
 * provided pointers, instead of the device's hardware mailbox. When the driver
 * calls mailbox functions like hqmv2_pf_write_vf_mbox_req(), the request will
 * go to the software mailbox instead of the hardware one. This is used in
 * S-IOV virtualization.
 */
void hqmv2_hw_register_sw_mbox(struct hqmv2_hw *hw,
			       unsigned int vdev_id,
			       u32 *vdev2pf_mbox,
			       u32 *pf2vdev_mbox,
			       void (*pf2vdev_inject)(void *),
			       void *inject_arg);

/**
 * hqmv2_hw_unregister_sw_mbox() - unregister a software mailbox
 * @hw: hqmv2_hw handle for a particular device.
 * @vdev_id: vdev ID.
 *
 * This function notifies the driver to stop using a previously registered
 * software mailbox.
 */
void hqmv2_hw_unregister_sw_mbox(struct hqmv2_hw *hw, unsigned int vdev_id);

/**
 * hqmv2_hw_setup_cq_ims_entry() - setup a CQ's IMS entry
 * @hw: hqmv2_hw handle for a particular device.
 * @vdev_id: vdev ID.
 * @virt_cq_id: virtual CQ ID.
 * @is_ldb: CQ is load-balanced.
 * @address_lo: least-significant 32 bits of address.
 * @data: 32 data bits.
 *
 * This sets up the CQ's IMS entry with the provided address and data values.
 * This function should only be called if the device is configured for S-IOV
 * virtualization. The upper 32 address bits are fixed in hardware and thus not
 * needed.
 */
void hqmv2_hw_setup_cq_ims_entry(struct hqmv2_hw *hw,
				 unsigned int vdev_id,
				 u32 virt_cq_id,
				 bool is_ldb,
				 u32 addr_lo,
				 u32 data);

/**
 * hqmv2_hw_clear_cq_ims_entry() - clear a CQ's IMS entry
 * @hw: hqmv2_hw handle for a particular device.
 * @vdev_id: vdev ID.
 * @virt_cq_id: virtual CQ ID.
 * @is_ldb: CQ is load-balanced.
 *
 * This clears the CQ's IMS entry, reverting it to its reset state.
 */
void hqmv2_hw_clear_cq_ims_entry(struct hqmv2_hw *hw,
				 unsigned int vdev_id,
				 u32 virt_cq_id,
				 bool is_ldb);

/**
 * hqmv2_hw_register_pasid() - register a vdev's PASID
 * @hw: hqmv2_hw handle for a particular device.
 * @vdev_id: vdev ID.
 * @pasid: the vdev's PASID.
 *
 * This function stores the user-supplied PASID, and uses it when configuring
 * the vdev's CQs.
 *
 * Return:
 * Returns >= 0 upon success, -1 otherwise.
 */
int hqmv2_hw_register_pasid(struct hqmv2_hw *hw,
			    unsigned int vdev_id,
			    unsigned int pasid);

/**
 * hqmv2_hw_pending_port_unmaps() - returns the number of unmap operations in
 *	progress.
 * @hw: hqmv2_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: number of unmaps in progress args
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum hqmv2_error. If successful, resp->id
 * contains the number of unmaps in progress.
 *
 * Errors:
 * EINVAL - Invalid port ID.
 */
int hqmv2_hw_pending_port_unmaps(struct hqmv2_hw *hw,
				 u32 domain_id,
				 struct hqmv2_pending_port_unmaps_args *args,
				 struct hqmv2_cmd_response *resp,
				 bool vf_request,
				 unsigned int vf_id);

/**
 * hqmv2_hw_get_cos_bandwidth() - returns the percent of bandwidth allocated
 *	to a port class-of-service.
 * @hw: hqmv2_hw handle for a particular device.
 * @cos_id: class-of-service ID.
 * @bandwidth: class-of-service bandwidth.
 *
 * Return:
 * Returns -EINVAL if cos_id is invalid, else the class' bandwidth allocation.
 */
int hqmv2_hw_get_cos_bandwidth(struct hqmv2_hw *hw, u32 cos_id);

/**
 * hqmv2_hw_set_cos_bandwidth() - set a bandwidth allocation percentage for a
 *	port class-of-service.
 * @hw: hqmv2_hw handle for a particular device.
 * @cos_id: class-of-service ID.
 * @bandwidth: class-of-service bandwidth.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise.
 *
 * Errors:
 * EINVAL - Invalid cos ID, bandwidth is greater than 100, or bandwidth would
 *	    cause the total bandwidth across all classes of service to exceed
 *	    100%.
 */
int hqmv2_hw_set_cos_bandwidth(struct hqmv2_hw *hw, u32 cos_id, u8 bandwidth);

#endif /* __HQMV2_BASE_HQMV2_API_H */
