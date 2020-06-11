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
 * dlb_resource_reset() - reset in-use resources to their initial state
 * @hw: dlb_hw handle for a particular device.
 *
 * This function resets in-use resources, and makes them available for use.
 * All resources go back to their owning function, whether a PF or a VF.
 */
void dlb_resource_reset(struct dlb_hw *hw);

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
			   u64 pop_count_dma_base,
			   u64 cq_dma_base,
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
			   u64 pop_count_dma_base,
			   u64 cq_dma_base,
			   struct dlb_cmd_response *resp,
			   bool vf_request,
			   unsigned int vf_id);

/**
 * dlb_hw_start_domain() - start a scheduling domain
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: start domain arguments.
 * @resp: response structure.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function starts a scheduling domain, which allows applications to send
 * traffic through it. Once a domain is started, its resources can no longer be
 * configured (besides QID remapping and port enable/disable).
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum dlb_error.
 *
 * Errors:
 * EINVAL - the domain is not configured, or the domain is already started.
 */
int dlb_hw_start_domain(struct dlb_hw *hw,
			u32 domain_id,
			struct dlb_start_domain_args *args,
			struct dlb_cmd_response *resp,
			bool vf_request,
			unsigned int vf_id);

/**
 * dlb_hw_map_qid() - map a load-balanced queue to a load-balanced port
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: map QID arguments.
 * @resp: response structure.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function configures the DLB to schedule QEs from the specified queue to
 * the specified port. Each load-balanced port can be mapped to up to 8 queues;
 * each load-balanced queue can potentially map to all the load-balanced ports.
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
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum dlb_error.
 *
 * Errors:
 * EINVAL - A requested resource is unavailable, invalid port or queue ID, or
 *	    the domain is not configured.
 * EFAULT - Internal error (resp->status not set).
 */
int dlb_hw_map_qid(struct dlb_hw *hw,
		   u32 domain_id,
		   struct dlb_map_qid_args *args,
		   struct dlb_cmd_response *resp,
		   bool vf_request,
		   unsigned int vf_id);

/**
 * dlb_hw_unmap_qid() - Unmap a load-balanced queue from a load-balanced port
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: unmap QID arguments.
 * @resp: response structure.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function configures the DLB to stop scheduling QEs from the specified
 * queue to the specified port.
 *
 * A successful return does not necessarily mean the mapping was removed. If
 * this function is unable to immediately unmap the queue from the port, it
 * will add the requested operation to a per-port list of pending map/unmap
 * operations, and (if it's not already running) launch a kernel thread that
 * periodically attempts to process all pending operations. See
 * dlb_hw_map_qid() for more details.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum dlb_error.
 *
 * Errors:
 * EINVAL - A requested resource is unavailable, invalid port or queue ID, or
 *	    the domain is not configured.
 * EFAULT - Internal error (resp->status not set).
 */
int dlb_hw_unmap_qid(struct dlb_hw *hw,
		     u32 domain_id,
		     struct dlb_unmap_qid_args *args,
		     struct dlb_cmd_response *resp,
		     bool vf_request,
		     unsigned int vf_id);

/**
 * dlb_hw_enable_ldb_port() - enable a load-balanced port for scheduling
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: port enable arguments.
 * @resp: response structure.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function configures the DLB to schedule QEs to a load-balanced port.
 * Ports are enabled by default.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum dlb_error.
 *
 * Errors:
 * EINVAL - The port ID is invalid or the domain is not configured.
 * EFAULT - Internal error (resp->status not set).
 */
int dlb_hw_enable_ldb_port(struct dlb_hw *hw,
			   u32 domain_id,
			   struct dlb_enable_ldb_port_args *args,
			   struct dlb_cmd_response *resp,
			   bool vf_request,
			   unsigned int vf_id);

/**
 * dlb_hw_disable_ldb_port() - disable a load-balanced port for scheduling
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: port disable arguments.
 * @resp: response structure.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function configures the DLB to stop scheduling QEs to a load-balanced
 * port. Ports are enabled by default.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum dlb_error.
 *
 * Errors:
 * EINVAL - The port ID is invalid or the domain is not configured.
 * EFAULT - Internal error (resp->status not set).
 */
int dlb_hw_disable_ldb_port(struct dlb_hw *hw,
			    u32 domain_id,
			    struct dlb_disable_ldb_port_args *args,
			    struct dlb_cmd_response *resp,
			    bool vf_request,
			    unsigned int vf_id);

/**
 * dlb_hw_enable_dir_port() - enable a directed port for scheduling
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: port enable arguments.
 * @resp: response structure.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function configures the DLB to schedule QEs to a directed port.
 * Ports are enabled by default.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum dlb_error.
 *
 * Errors:
 * EINVAL - The port ID is invalid or the domain is not configured.
 * EFAULT - Internal error (resp->status not set).
 */
int dlb_hw_enable_dir_port(struct dlb_hw *hw,
			   u32 domain_id,
			   struct dlb_enable_dir_port_args *args,
			   struct dlb_cmd_response *resp,
			   bool vf_request,
			   unsigned int vf_id);

/**
 * dlb_hw_disable_dir_port() - disable a directed port for scheduling
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: port disable arguments.
 * @resp: response structure.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function configures the DLB to stop scheduling QEs to a directed port.
 * Ports are enabled by default.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum dlb_error.
 *
 * Errors:
 * EINVAL - The port ID is invalid or the domain is not configured.
 * EFAULT - Internal error (resp->status not set).
 */
int dlb_hw_disable_dir_port(struct dlb_hw *hw,
			    u32 domain_id,
			    struct dlb_disable_dir_port_args *args,
			    struct dlb_cmd_response *resp,
			    bool vf_request,
			    unsigned int vf_id);

/**
 * dlb_configure_ldb_cq_interrupt() - configure load-balanced CQ for interrupts
 * @hw: dlb_hw handle for a particular device.
 * @port_id: load-balancd port ID.
 * @vector: interrupt vector ID. Should be 0 for MSI or compressed MSI-X mode,
 *	    else a value up to 64.
 * @mode: interrupt type (DLB_CQ_ISR_MODE_MSI or DLB_CQ_ISR_MODE_MSIX)
 * @vf: If the port is VF-owned, the VF's ID. This is used for translating the
 *	virtual port ID to a physical port ID. Ignored if mode is not MSI.
 * @owner_vf: the VF to route the interrupt to. Ignore if mode is not MSI.
 * @threshold: the minimum CQ depth at which the interrupt can fire. Must be
 *	greater than 0.
 *
 * This function configures the DLB registers for load-balanced CQ's interrupts.
 * This doesn't enable the CQ's interrupt; that can be done with
 * dlb_arm_cq_interrupt() or through an interrupt arm QE.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise.
 *
 * Errors:
 * EINVAL - The port ID is invalid.
 */
int dlb_configure_ldb_cq_interrupt(struct dlb_hw *hw,
				   int port_id,
				   int vector,
				   int mode,
				   unsigned int vf,
				   unsigned int owner_vf,
				   u16 threshold);

/**
 * dlb_configure_dir_cq_interrupt() - configure directed CQ for interrupts
 * @hw: dlb_hw handle for a particular device.
 * @port_id: load-balancd port ID.
 * @vector: interrupt vector ID. Should be 0 for MSI or compressed MSI-X mode,
 *	    else a value up to 64.
 * @mode: interrupt type (DLB_CQ_ISR_MODE_MSI or DLB_CQ_ISR_MODE_MSIX)
 * @vf: If the port is VF-owned, the VF's ID. This is used for translating the
 *	virtual port ID to a physical port ID. Ignored if mode is not MSI.
 * @owner_vf: the VF to route the interrupt to. Ignore if mode is not MSI.
 * @threshold: the minimum CQ depth at which the interrupt can fire. Must be
 *	greater than 0.
 *
 * This function configures the DLB registers for directed CQ's interrupts.
 * This doesn't enable the CQ's interrupt; that can be done with
 * dlb_arm_cq_interrupt() or through an interrupt arm QE.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise.
 *
 * Errors:
 * EINVAL - The port ID is invalid.
 */
int dlb_configure_dir_cq_interrupt(struct dlb_hw *hw,
				   int port_id,
				   int vector,
				   int mode,
				   unsigned int vf,
				   unsigned int owner_vf,
				   u16 threshold);

/**
 * dlb_enable_alarm_interrupts() - enable certain hardware alarm interrupts
 * @hw: dlb_hw handle for a particular device.
 *
 * This function configures the ingress error alarm. (Other alarms are enabled
 * by default.)
 */
void dlb_enable_alarm_interrupts(struct dlb_hw *hw);

/**
 * dlb_disable_alarm_interrupts() - disable certain hardware alarm interrupts
 * @hw: dlb_hw handle for a particular device.
 *
 * This function configures the ingress error alarm. (Other alarms are disabled
 * by default.)
 */
void dlb_disable_alarm_interrupts(struct dlb_hw *hw);

/**
 * dlb_set_msix_mode() - enable certain hardware alarm interrupts
 * @hw: dlb_hw handle for a particular device.
 * @mode: MSI-X mode (DLB_MSIX_MODE_PACKED or DLB_MSIX_MODE_COMPRESSED)
 *
 * This function configures the hardware to use either packed or compressed
 * mode. This function should not be called if using MSI interrupts.
 */
void dlb_set_msix_mode(struct dlb_hw *hw, int mode);

/**
 * dlb_arm_cq_interrupt() - arm a CQ's interrupt
 * @hw: dlb_hw handle for a particular device.
 * @port_id: port ID
 * @is_ldb: true for load-balanced port, false for a directed port
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function arms the CQ's interrupt. The CQ must be configured prior to
 * calling this function.
 *
 * The function does no parameter validation; that is the caller's
 * responsibility.
 *
 * Return: returns 0 upon success, <0 otherwise.
 *
 * EINVAL - Invalid port ID.
 */
int dlb_arm_cq_interrupt(struct dlb_hw *hw,
			 int port_id,
			 bool is_ldb,
			 bool vf_request,
			 unsigned int vf_id);

/**
 * dlb_read_compressed_cq_intr_status() - read compressed CQ interrupt status
 * @hw: dlb_hw handle for a particular device.
 * @ldb_interrupts: 2-entry array of u32 bitmaps
 * @dir_interrupts: 4-entry array of u32 bitmaps
 *
 * This function can be called from a compressed CQ interrupt handler to
 * determine which CQ interrupts have fired. The caller should take appropriate
 * (such as waking threads blocked on a CQ's interrupt) then ack the interrupts
 * with dlb_ack_compressed_cq_intr().
 */
void dlb_read_compressed_cq_intr_status(struct dlb_hw *hw,
					u32 *ldb_interrupts,
					u32 *dir_interrupts);

/**
 * dlb_ack_compressed_cq_intr_status() - ack compressed CQ interrupts
 * @hw: dlb_hw handle for a particular device.
 * @ldb_interrupts: 2-entry array of u32 bitmaps
 * @dir_interrupts: 4-entry array of u32 bitmaps
 *
 * This function ACKs compressed CQ interrupts. Its arguments should be the
 * same ones passed to dlb_read_compressed_cq_intr_status().
 */
void dlb_ack_compressed_cq_intr(struct dlb_hw *hw,
				u32 *ldb_interrupts,
				u32 *dir_interrupts);

/**
 * dlb_read_vf_intr_status() - read the VF interrupt status register
 * @hw: dlb_hw handle for a particular device.
 *
 * This function can be called from a VF's interrupt handler to determine
 * which interrupts have fired. The first 31 bits correspond to CQ interrupt
 * vectors, and the final bit is for the PF->VF mailbox interrupt vector.
 *
 * Return:
 * Returns a bit vector indicating which interrupt vectors are active.
 */
u32 dlb_read_vf_intr_status(struct dlb_hw *hw);

/**
 * dlb_ack_vf_intr_status() - ack VF interrupts
 * @hw: dlb_hw handle for a particular device.
 * @interrupts: 32-bit bitmap
 *
 * This function ACKs a VF's interrupts. Its interrupts argument should be the
 * value returned by dlb_read_vf_intr_status().
 */
void dlb_ack_vf_intr_status(struct dlb_hw *hw, u32 interrupts);

/**
 * dlb_ack_vf_msi_intr() - ack VF MSI interrupt
 * @hw: dlb_hw handle for a particular device.
 * @interrupts: 32-bit bitmap
 *
 * This function clears the VF's MSI interrupt pending register. Its interrupts
 * argument should be contain the MSI vectors to ACK. For example, if MSI MME
 * is in mode 0, then one bit 0 should ever be set.
 */
void dlb_ack_vf_msi_intr(struct dlb_hw *hw, u32 interrupts);

/**
 * dlb_ack_vf_mbox_int() - ack PF->VF mailbox interrupt
 * @hw: dlb_hw handle for a particular device.
 *
 * When done processing the PF mailbox request, this function unsets
 * the PF's mailbox ISR register.
 */
void dlb_ack_pf_mbox_int(struct dlb_hw *hw);

/**
 * dlb_read_vf_to_pf_int_bitvec() - return a bit vector of all requesting VFs
 * @hw: dlb_hw handle for a particular device.
 *
 * When the VF->PF ISR fires, this function can be called to determine which
 * VF(s) are requesting service. This bitvector must be passed to
 * dlb_ack_vf_to_pf_int() when processing is complete for all requesting VFs.
 *
 * Return:
 * Returns a bit vector indicating which VFs (0-15) have requested service.
 */
u32 dlb_read_vf_to_pf_int_bitvec(struct dlb_hw *hw);

/**
 * dlb_ack_vf_mbox_int() - ack processed VF->PF mailbox interrupt
 * @hw: dlb_hw handle for a particular device.
 * @bitvec: bit vector returned by dlb_read_vf_to_pf_int_bitvec()
 *
 * When done processing all VF mailbox requests, this function unsets the VF's
 * mailbox ISR register.
 */
void dlb_ack_vf_mbox_int(struct dlb_hw *hw, u32 bitvec);

/**
 * dlb_read_vf_flr_int_bitvec() - return a bit vector of all VFs requesting FLR
 * @hw: dlb_hw handle for a particular device.
 *
 * When the VF FLR ISR fires, this function can be called to determine which
 * VF(s) are requesting FLRs. This bitvector must passed to
 * dlb_ack_vf_flr_int() when processing is complete for all requesting VFs.
 *
 * Return:
 * Returns a bit vector indicating which VFs (0-15) have requested FLRs.
 */
u32 dlb_read_vf_flr_int_bitvec(struct dlb_hw *hw);

/**
 * dlb_ack_vf_flr_int() - ack processed VF<->PF interrupt(s)
 * @hw: dlb_hw handle for a particular device.
 * @bitvec: bit vector returned by dlb_read_vf_flr_int_bitvec()
 * @a_stepping: device is A-stepping
 *
 * When done processing all VF FLR requests, this function unsets the VF's FLR
 * ISR register.
 *
 * Note: The caller must ensure dlb_set_vf_reset_in_progress(),
 * dlb_clr_vf_reset_in_progress(), and dlb_ack_vf_flr_int() are not executed in
 * parallel, because the reset-in-progress register does not support atomic
 * updates on A-stepping devices.
 */
void dlb_ack_vf_flr_int(struct dlb_hw *hw, u32 bitvec, bool a_stepping);

/**
 * dlb_ack_vf_to_pf_int() - ack processed VF mbox and FLR interrupt(s)
 * @hw: dlb_hw handle for a particular device.
 * @mbox_bitvec: bit vector returned by dlb_read_vf_to_pf_int_bitvec()
 * @flr_bitvec: bit vector returned by dlb_read_vf_flr_int_bitvec()
 *
 * When done processing all VF requests, this function communicates to the
 * hardware that processing is complete. When this function completes, hardware
 * can immediately generate another VF mbox or FLR interrupt.
 */
void dlb_ack_vf_to_pf_int(struct dlb_hw *hw,
			  u32 mbox_bitvec,
			  u32 flr_bitvec);

/**
 * dlb_process_alarm_interrupt() - process an alarm interrupt
 * @hw: dlb_hw handle for a particular device.
 *
 * This function reads the alarm syndrome, logs its, and acks the interrupt.
 * This function should be called from the alarm interrupt handler when
 * interrupt vector DLB_INT_ALARM fires.
 */
void dlb_process_alarm_interrupt(struct dlb_hw *hw);

/**
 * dlb_process_ingress_error_interrupt() - process ingress error interrupts
 * @hw: dlb_hw handle for a particular device.
 *
 * This function reads the alarm syndrome, logs it, notifies user-space, and
 * acks the interrupt. This function should be called from the alarm interrupt
 * handler when interrupt vector DLB_INT_INGRESS_ERROR fires.
 */
void dlb_process_ingress_error_interrupt(struct dlb_hw *hw);

/**
 * dlb_get_group_sequence_numbers() - return a group's number of SNs per queue
 * @hw: dlb_hw handle for a particular device.
 * @group_id: sequence number group ID.
 *
 * This function returns the configured number of sequence numbers per queue
 * for the specified group.
 *
 * Return:
 * Returns -EINVAL if group_id is invalid, else the group's SNs per queue.
 */
int dlb_get_group_sequence_numbers(struct dlb_hw *hw, unsigned int group_id);

/**
 * dlb_get_group_sequence_number_occupancy() - return a group's in-use slots
 * @hw: dlb_hw handle for a particular device.
 * @group_id: sequence number group ID.
 *
 * This function returns the group's number of in-use slots (i.e. load-balanced
 * queues using the specified group).
 *
 * Return:
 * Returns -EINVAL if group_id is invalid, else the group's occupancy.
 */
int dlb_get_group_sequence_number_occupancy(struct dlb_hw *hw,
					    unsigned int group_id);

/**
 * dlb_set_group_sequence_numbers() - assign a group's number of SNs per queue
 * @hw: dlb_hw handle for a particular device.
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
int dlb_set_group_sequence_numbers(struct dlb_hw *hw,
				   unsigned int group_id,
				   unsigned long val);

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
 * dlb_hw_get_num_used_resources() - query the PCI function's used resources
 * @arg: pointer to resource counts.
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * This function returns the number of resources in use by the PF or a VF. It
 * fills in the fields that args points to, except the following:
 * - max_contiguous_atomic_inflights
 * - max_contiguous_hist_list_entries
 * - max_contiguous_ldb_credits
 * - max_contiguous_dir_credits
 *
 * Return:
 * Returns 0 upon success, -EINVAL if vf_request is true and vf_id is invalid.
 */
int dlb_hw_get_num_used_resources(struct dlb_hw *hw,
				  struct dlb_get_num_resources_args *arg,
				  bool vf_request,
				  unsigned int vf_id);

/**
 * dlb_send_async_vf_to_pf_msg() - (VF only) send a mailbox message to the PF
 * @hw: dlb_hw handle for a particular device.
 *
 * This function sends a VF->PF mailbox message. It is asynchronous, so it
 * returns once the message is sent but potentially before the PF has processed
 * the message. The caller must call dlb_vf_to_pf_complete() to determine when
 * the PF has finished processing the request.
 */
void dlb_send_async_vf_to_pf_msg(struct dlb_hw *hw);

/**
 * dlb_vf_to_pf_complete() - check the status of an asynchronous mailbox request
 * @hw: dlb_hw handle for a particular device.
 *
 * This function returns a boolean indicating whether the PF has finished
 * processing a VF->PF mailbox request. It should only be called after sending
 * an asynchronous request with dlb_send_async_vf_to_pf_msg().
 */
bool dlb_vf_to_pf_complete(struct dlb_hw *hw);

/**
 * dlb_vf_flr_complete() - check the status of a VF FLR
 * @hw: dlb_hw handle for a particular device.
 *
 * This function returns a boolean indicating whether the PF has finished
 * executing the VF FLR. It should only be called after setting the VF's FLR
 * bit.
 */
bool dlb_vf_flr_complete(struct dlb_hw *hw);

/**
 * dlb_set_vf_reset_in_progress() - set a VF's reset in progress bit
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID.
 *
 * Note: This function is only supported on A-stepping devices.
 *
 * Note: The caller must ensure dlb_set_vf_reset_in_progress(),
 * dlb_clr_vf_reset_in_progress(), and dlb_ack_vf_flr_int() are not executed in
 * parallel, because the reset-in-progress register does not support atomic
 * updates on A-stepping devices.
 */
void dlb_set_vf_reset_in_progress(struct dlb_hw *hw, int vf_id);

/**
 * dlb_clr_vf_reset_in_progress() - clear a VF's reset in progress bit
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID.
 *
 * Note: This function is only supported on A-stepping devices.
 *
 * Note: The caller must ensure dlb_set_vf_reset_in_progress(),
 * dlb_clr_vf_reset_in_progress(), and dlb_ack_vf_flr_int() are not executed in
 * parallel, because the reset-in-progress register does not support atomic
 * updates on A-stepping devices.
 */
void dlb_clr_vf_reset_in_progress(struct dlb_hw *hw, int vf_id);

/**
 * dlb_send_async_pf_to_vf_msg() - (PF only) send a mailbox message to the VF
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID.
 *
 * This function sends a PF->VF mailbox message. It is asynchronous, so it
 * returns once the message is sent but potentially before the VF has processed
 * the message. The caller must call dlb_pf_to_vf_complete() to determine when
 * the VF has finished processing the request.
 */
void dlb_send_async_pf_to_vf_msg(struct dlb_hw *hw, unsigned int vf_id);

/**
 * dlb_pf_to_vf_complete() - check the status of an asynchronous mailbox request
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID.
 *
 * This function returns a boolean indicating whether the VF has finished
 * processing a PF->VF mailbox request. It should only be called after sending
 * an asynchronous request with dlb_send_async_pf_to_vf_msg().
 */
bool dlb_pf_to_vf_complete(struct dlb_hw *hw, unsigned int vf_id);

/**
 * dlb_pf_read_vf_mbox_req() - (PF only) read a VF->PF mailbox request
 * @hw: dlb_hw handle for a particular device.
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
 * EINVAL - len >= DLB_VF2PF_REQ_BYTES.
 */
int dlb_pf_read_vf_mbox_req(struct dlb_hw *hw,
			    unsigned int vf_id,
			    void *data,
			    int len);

/**
 * dlb_pf_read_vf_mbox_resp() - (PF only) read a VF->PF mailbox response
 * @hw: dlb_hw handle for a particular device.
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
 * EINVAL - len >= DLB_VF2PF_RESP_BYTES.
 */
int dlb_pf_read_vf_mbox_resp(struct dlb_hw *hw,
			     unsigned int vf_id,
			     void *data,
			     int len);

/**
 * dlb_pf_write_vf_mbox_resp() - (PF only) write a PF->VF mailbox response
 * @hw: dlb_hw handle for a particular device.
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
 * EINVAL - len >= DLB_PF2VF_RESP_BYTES.
 */
int dlb_pf_write_vf_mbox_resp(struct dlb_hw *hw,
			      unsigned int vf_id,
			      void *data,
			      int len);

/**
 * dlb_pf_write_vf_mbox_req() - (PF only) write a PF->VF mailbox request
 * @hw: dlb_hw handle for a particular device.
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
 * EINVAL - len >= DLB_PF2VF_REQ_BYTES.
 */
int dlb_pf_write_vf_mbox_req(struct dlb_hw *hw,
			     unsigned int vf_id,
			     void *data,
			     int len);

/**
 * dlb_vf_read_pf_mbox_resp() - (VF only) read a PF->VF mailbox response
 * @hw: dlb_hw handle for a particular device.
 * @data: pointer to message data.
 * @len: size, in bytes, of the data array.
 *
 * This function copies the VF's PF->VF mailbox into the array pointed to by
 * data.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * EINVAL - len >= DLB_PF2VF_RESP_BYTES.
 */
int dlb_vf_read_pf_mbox_resp(struct dlb_hw *hw, void *data, int len);

/**
 * dlb_vf_read_pf_mbox_req() - (VF only) read a PF->VF mailbox request
 * @hw: dlb_hw handle for a particular device.
 * @data: pointer to message data.
 * @len: size, in bytes, of the data array.
 *
 * This function copies the VF's PF->VF mailbox into the array pointed to by
 * data.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * EINVAL - len >= DLB_PF2VF_REQ_BYTES.
 */
int dlb_vf_read_pf_mbox_req(struct dlb_hw *hw, void *data, int len);

/**
 * dlb_vf_write_pf_mbox_req() - (VF only) write a VF->PF mailbox request
 * @hw: dlb_hw handle for a particular device.
 * @data: pointer to message data.
 * @len: size, in bytes, of the data array.
 *
 * This function copies the user-provided message data into of the VF's PF->VF
 * mailboxes.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * EINVAL - len >= DLB_VF2PF_REQ_BYTES.
 */
int dlb_vf_write_pf_mbox_req(struct dlb_hw *hw, void *data, int len);

/**
 * dlb_vf_write_pf_mbox_resp() - (VF only) write a VF->PF mailbox response
 * @hw: dlb_hw handle for a particular device.
 * @data: pointer to message data.
 * @len: size, in bytes, of the data array.
 *
 * This function copies the user-provided message data into of the VF's PF->VF
 * mailboxes.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * EINVAL - len >= DLB_VF2PF_RESP_BYTES.
 */
int dlb_vf_write_pf_mbox_resp(struct dlb_hw *hw, void *data, int len);

/**
 * dlb_reset_vf() - reset the hardware owned by a VF
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID
 *
 * This function resets the hardware owned by a VF (if any), by resetting the
 * VF's domains one by one.
 */
int dlb_reset_vf(struct dlb_hw *hw, unsigned int vf_id);

/**
 * dlb_vf_is_locked() - check whether the VF's resources are locked
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID
 *
 * This function returns whether or not the VF's resource assignments are
 * locked. If locked, no resources can be added to or subtracted from the
 * group.
 */
bool dlb_vf_is_locked(struct dlb_hw *hw, unsigned int vf_id);

/**
 * dlb_lock_vf() - lock the VF's resources
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID
 *
 * This function sets a flag indicating that the VF is using its resources.
 * When VF is locked, its resource assignment cannot be changed.
 */
void dlb_lock_vf(struct dlb_hw *hw, unsigned int vf_id);

/**
 * dlb_unlock_vf() - unlock the VF's resources
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID
 *
 * This function unlocks the VF's resource assignment, allowing it to be
 * modified.
 */
void dlb_unlock_vf(struct dlb_hw *hw, unsigned int vf_id);

/**
 * dlb_update_vf_sched_domains() - update the domains assigned to a VF
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID
 * @num: number of scheduling domains to assign to this VF
 *
 * This function assigns num scheduling domains to the specified VF. If the VF
 * already has domains assigned, this existing assignment is adjusted
 * accordingly.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - vf_id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The VF's resource assignment is locked and cannot be changed.
 */
int dlb_update_vf_sched_domains(struct dlb_hw *hw,
				u32 vf_id,
				u32 num);

/**
 * dlb_update_vf_ldb_queues() - update the LDB queues assigned to a VF
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID
 * @num: number of LDB queues to assign to this VF
 *
 * This function assigns num LDB queues to the specified VF. If the VF already
 * has LDB queues assigned, this existing assignment is adjusted
 * accordingly.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - vf_id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The VF's resource assignment is locked and cannot be changed.
 */
int dlb_update_vf_ldb_queues(struct dlb_hw *hw, u32 vf_id, u32 num);

/**
 * dlb_update_vf_ldb_ports() - update the LDB ports assigned to a VF
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID
 * @num: number of LDB ports to assign to this VF
 *
 * This function assigns num LDB ports to the specified VF. If the VF already
 * has LDB ports assigned, this existing assignment is adjusted accordingly.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - vf_id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The VF's resource assignment is locked and cannot be changed.
 */
int dlb_update_vf_ldb_ports(struct dlb_hw *hw, u32 vf_id, u32 num);

/**
 * dlb_update_vf_dir_ports() - update the DIR ports assigned to a VF
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID
 * @num: number of DIR ports to assign to this VF
 *
 * This function assigns num DIR ports to the specified VF. If the VF already
 * has DIR ports assigned, this existing assignment is adjusted accordingly.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - vf_id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The VF's resource assignment is locked and cannot be changed.
 */
int dlb_update_vf_dir_ports(struct dlb_hw *hw, u32 vf_id, u32 num);

/**
 * dlb_update_vf_ldb_credit_pools() - update the VF's assigned LDB pools
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID
 * @num: number of LDB credit pools to assign to this VF
 *
 * This function assigns num LDB credit pools to the specified VF. If the VF
 * already has LDB credit pools assigned, this existing assignment is adjusted
 * accordingly.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - vf_id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The VF's resource assignment is locked and cannot be changed.
 */
int dlb_update_vf_ldb_credit_pools(struct dlb_hw *hw,
				   u32 vf_id,
				   u32 num);

/**
 * dlb_update_vf_dir_credit_pools() - update the VF's assigned DIR pools
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID
 * @num: number of DIR credit pools to assign to this VF
 *
 * This function assigns num DIR credit pools to the specified VF. If the VF
 * already has DIR credit pools assigned, this existing assignment is adjusted
 * accordingly.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - vf_id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The VF's resource assignment is locked and cannot be changed.
 */
int dlb_update_vf_dir_credit_pools(struct dlb_hw *hw,
				   u32 vf_id,
				   u32 num);

/**
 * dlb_update_vf_ldb_credits() - update the VF's assigned LDB credits
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID
 * @num: number of LDB credits to assign to this VF
 *
 * This function assigns num LDB credits to the specified VF. If the VF already
 * has LDB credits assigned, this existing assignment is adjusted accordingly.
 * VF's are assigned a contiguous chunk of credits, so this function may fail
 * if a sufficiently large contiguous chunk is not available.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - vf_id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The VF's resource assignment is locked and cannot be changed.
 */
int dlb_update_vf_ldb_credits(struct dlb_hw *hw, u32 vf_id, u32 num);

/**
 * dlb_update_vf_dir_credits() - update the VF's assigned DIR credits
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID
 * @num: number of DIR credits to assign to this VF
 *
 * This function assigns num DIR credits to the specified VF. If the VF already
 * has DIR credits assigned, this existing assignment is adjusted accordingly.
 * VF's are assigned a contiguous chunk of credits, so this function may fail
 * if a sufficiently large contiguous chunk is not available.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - vf_id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The VF's resource assignment is locked and cannot be changed.
 */
int dlb_update_vf_dir_credits(struct dlb_hw *hw, u32 vf_id, u32 num);

/**
 * dlb_update_vf_hist_list_entries() - update the VF's assigned HL entries
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID
 * @num: number of history list entries to assign to this VF
 *
 * This function assigns num history list entries to the specified VF. If the
 * VF already has history list entries assigned, this existing assignment is
 * adjusted accordingly. VF's are assigned a contiguous chunk of entries, so
 * this function may fail if a sufficiently large contiguous chunk is not
 * available.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - vf_id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The VF's resource assignment is locked and cannot be changed.
 */
int dlb_update_vf_hist_list_entries(struct dlb_hw *hw,
				    u32 vf_id,
				    u32 num);

/**
 * dlb_update_vf_atomic_inflights() - update the VF's atomic inflights
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID
 * @num: number of atomic inflights to assign to this VF
 *
 * This function assigns num atomic inflights to the specified VF. If the VF
 * already has atomic inflights assigned, this existing assignment is adjusted
 * accordingly. VF's are assigned a contiguous chunk of entries, so this
 * function may fail if a sufficiently large contiguous chunk is not available.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - vf_id is invalid, or the requested number of resources are
 *	    unavailable.
 * EPERM  - The VF's resource assignment is locked and cannot be changed.
 */
int dlb_update_vf_atomic_inflights(struct dlb_hw *hw,
				   u32 vf_id,
				   u32 num);

/**
 * dlb_reset_vf_resources() - reassign the VF's resources to the PF
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID
 *
 * This function takes any resources currently assigned to the VF and reassigns
 * them to the PF.
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - vf_id is invalid
 * EPERM  - The VF's resource assignment is locked and cannot be changed.
 */
int dlb_reset_vf_resources(struct dlb_hw *hw, unsigned int vf_id);

/**
 * dlb_notify_vf() - send a notification to a VF
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID
 * @notification: notification
 *
 * This function sends a notification (as defined in dlb_mbox.h) to a VF.
 *
 * Return:
 * Returns 0 upon success, -1 if the VF doesn't ACK the PF->VF interrupt.
 */
int dlb_notify_vf(struct dlb_hw *hw,
		  unsigned int vf_id,
		  u32 notification);

/**
 * dlb_vf_in_use() - query whether a VF is in use
 * @hw: dlb_hw handle for a particular device.
 * @vf_id: VF ID
 *
 * This function sends a mailbox request to the VF to query whether the VF is in
 * use.
 *
 * Return:
 * Returns 0 for false, 1 for true, and -1 if the mailbox request times out or
 * an internal error occurs.
 */
int dlb_vf_in_use(struct dlb_hw *hw, unsigned int vf_id);

/**
 * dlb_disable_dp_vasr_feature() - disable directed pipe VAS reset hardware
 * @hw: dlb_hw handle for a particular device.
 *
 * This function disables certain hardware in the directed pipe,
 * necessary to workaround a DLB VAS reset issue.
 */
void dlb_disable_dp_vasr_feature(struct dlb_hw *hw);

/**
 * dlb_enable_excess_tokens_alarm() - enable interrupts for the excess token
 * pop alarm
 * @hw: dlb_hw handle for a particular device.
 *
 * This function enables the PF ingress error alarm interrupt to fire when an
 * excess token pop occurs.
 */
void dlb_enable_excess_tokens_alarm(struct dlb_hw *hw);

/**
 * dlb_disable_excess_tokens_alarm() - disable interrupts for the excess token
 * pop alarm
 * @hw: dlb_hw handle for a particular device.
 *
 * This function disables the PF ingress error alarm interrupt to fire when an
 * excess token pop occurs.
 */
void dlb_disable_excess_tokens_alarm(struct dlb_hw *hw);

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
 * dlb_hw_pending_port_unmaps() - returns the number of unmap operations in
 *	progress for a load-balanced port.
 * @hw: dlb_hw handle for a particular device.
 * @domain_id: domain ID.
 * @args: number of unmaps in progress args
 * @vf_request: indicates whether this request came from a VF.
 * @vf_id: If vf_request is true, this contains the VF's ID.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise. If an error occurs, resp->status is
 * assigned a detailed error code from enum dlb_error. If successful, resp->id
 * contains the number of unmaps in progress.
 *
 * Errors:
 * EINVAL - Invalid port ID.
 */
int dlb_hw_pending_port_unmaps(struct dlb_hw *hw,
			       u32 domain_id,
			       struct dlb_pending_port_unmaps_args *args,
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

/**
 * dlb_hw_set_qe_arbiter_weights() - program QE arbiter weights
 * @hw: dlb_hw handle for a particular device.
 * @weight: 8-entry array of arbiter weights.
 *
 * weight[N] programs priority N's weight. In cases where the 8 priorities are
 * reduced to 4 bins, the mapping is:
 * - weight[1] programs bin 0
 * - weight[3] programs bin 1
 * - weight[5] programs bin 2
 * - weight[7] programs bin 3
 */
void dlb_hw_set_qe_arbiter_weights(struct dlb_hw *hw, u8 weight[8]);

/**
 * dlb_hw_set_qid_arbiter_weights() - program QID arbiter weights
 * @hw: dlb_hw handle for a particular device.
 * @weight: 8-entry array of arbiter weights.
 *
 * weight[N] programs priority N's weight. In cases where the 8 priorities are
 * reduced to 4 bins, the mapping is:
 * - weight[1] programs bin 0
 * - weight[3] programs bin 1
 * - weight[5] programs bin 2
 * - weight[7] programs bin 3
 */
void dlb_hw_set_qid_arbiter_weights(struct dlb_hw *hw, u8 weight[8]);

/**
 * dlb_hw_enable_pp_sw_alarms() - enable out-of-credit alarm for all producer
 * ports
 * @hw: dlb_hw handle for a particular device.
 */
void dlb_hw_enable_pp_sw_alarms(struct dlb_hw *hw);

/**
 * dlb_hw_disable_pp_sw_alarms() - disable out-of-credit alarm for all producer
 * ports
 * @hw: dlb_hw handle for a particular device.
 */
void dlb_hw_disable_pp_sw_alarms(struct dlb_hw *hw);

#endif /* __DLB_RESOURCE_H */
