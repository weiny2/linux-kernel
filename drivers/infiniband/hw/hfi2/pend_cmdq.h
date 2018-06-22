/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2016 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright (c) 2016 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */
#ifndef _PEND_CMDQ_H
#define _PEND_CMDQ_H

struct hfi_pt_alloc_eager_args;

/**
 * hfi_pend_queue - Privileged command queue information
 * @event: Wait queue for kthread handling privileged CMDQ commands
 * @pending: Pending linked list of struct hfi_pend_cmd
 * @lock: spinlock to safely handle list add/remove
 * @cache: kmem_cache to allocate hfi_pend_cmd structs
 * @thread: thread to handle privileged CMDQ writes
 * @dd: Back pointer to device data
 */
struct hfi_pend_queue {
	wait_queue_head_t event;
	struct list_head pending;
	/* Lock protects pending list access */
	spinlock_t lock;
	struct kmem_cache *cache;
	struct task_struct *thread;
	struct hfi_devdata *dd;
};

int hfi_pend_cmdq_info_alloc(struct hfi_devdata *dd, struct hfi_pend_queue *pq,
			     char *name);
void hfi_pend_cmdq_info_free(struct hfi_pend_queue *pq);

/**
 * _hfi_pend_cmd_queue - queue a CMDQ write
 * @pq: pending queue pointer
 * @cmdq: the CMDQ to write into
 * @eq: EQ to check before writing CMDQ command
 * @slots: pointer to the slot data
 * @cmd_slots: number of slots
 * @wait: optionally wait until the CMDQ command has been written
 * @gfp: gfp flags passed by caller to use for allocating cache object
 */
int _hfi_pend_cmd_queue(struct hfi_pend_queue *pq, struct hfi_cmdq *cmdq,
			struct hfi_eq *eq, void *slots, int cmd_slots,
			bool wait, gfp_t gfp);

#define hfi_pend_cmd_queue_wait(pq, cmdq, eq, slots, cmd_slots) \
	_hfi_pend_cmd_queue(pq, cmdq, eq, slots, cmd_slots, true, GFP_KERNEL)

#define hfi_pend_cmd_queue(pq, cmdq, eq, slots, cmd_slots, gfp) \
	_hfi_pend_cmd_queue(pq, cmdq, eq, slots, cmd_slots, false, gfp)

int hfi_pt_update_pending(struct hfi_pend_queue *pq, struct hfi_ctx *ctx,
			  struct hfi_cmdq *rx_cmdq, u16 eager_head);

int hfi_pt_disable_pending(struct hfi_pend_queue *pq, struct hfi_ctx *ctx,
			   struct hfi_cmdq *rx_cmdq, u8 ni, u32 pt_idx);

int hfi_pt_alloc_eager_pending(struct hfi_pend_queue *pq, struct hfi_ctx *ctx,
			       struct hfi_cmdq *rx_cmdq,
			       struct hfi_pt_alloc_eager_args *args);

#endif
